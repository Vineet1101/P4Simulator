/* Copyright 2024 Marvell Technology, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Rupesh Chiluka (rchiluka@marvell.com)
 *
 */

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF

#ifdef Mutex
#undef Mutex
#endif

// Undefine conflicting macro if it exists
#ifdef registry_t
#undef registry_t
#endif

#include <bm/spdlog/spdlog.h>
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_DEBUG

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "p4-nic-pna.h"

#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/tables.h>

namespace ns3 {

uint64_t P4PnaNic::packet_id = 0;

P4PnaNic::P4PnaNic (bool enable_swap)
    : bm::Switch (enable_swap),
      input_buffer (1024),
      start_timestamp (Simulator::Now ().GetNanoSeconds ())
{
  add_required_field ("pna_main_parser_input_metadata", "recirculated");
  add_required_field ("pna_main_parser_input_metadata", "input_port");

  add_required_field ("pna_main_input_metadata", "recirculated");
  add_required_field ("pna_main_input_metadata", "timestamp");
  add_required_field ("pna_main_input_metadata", "parser_error");
  add_required_field ("pna_main_input_metadata", "class_of_service");
  add_required_field ("pna_main_input_metadata", "input_port");

  add_required_field ("pna_main_output_metadata", "class_of_service");

  force_arith_header ("pna_main_parser_input_metadata");
  force_arith_header ("pna_main_input_metadata");
  force_arith_header ("pna_main_output_metadata");
}

P4PnaNic::~P4PnaNic ()
{
  input_buffer.push_front (nullptr);
}

void
P4PnaNic::reset_target_state_ ()
{
  NS_LOG_FUNCTION (this);
  get_component<bm::McSimplePreLAG> ()->reset_state ();
}

uint64_t
P4PnaNic::GetTimeStamp ()
{
  return Simulator::Now ().GetNanoSeconds () - start_timestamp;
}

void
P4PnaNic::main_thread ()
{
  bm::PHV *phv;

  while (1)
    {
      std::unique_ptr<bm::Packet> packet;
      input_buffer.pop_back (&packet);
      if (packet == nullptr)
        break;

      phv = packet->get_phv ();
      auto input_port = phv->get_field ("pna_main_parser_input_metadata.input_port").get_uint ();

      NS_LOG_DEBUG ("Processing packet received on port " << input_port);

      phv->get_field ("pna_main_input_metadata.timestamp").set (GetTimeStamp ());

      bm::Parser *parser = this->get_parser ("main_parser");
      parser->parse (packet.get ());

      // pass relevant values from main parser
      phv->get_field ("pna_main_input_metadata.recirculated")
          .set (phv->get_field ("pna_main_parser_input_metadata.recirculated"));
      phv->get_field ("pna_main_input_metadata.parser_error")
          .set (packet->get_error_code ().get ());
      phv->get_field ("pna_main_input_metadata.class_of_service").set (0);
      phv->get_field ("pna_main_input_metadata.input_port")
          .set (phv->get_field ("pna_main_parser_input_metadata.input_port"));

      bm::Pipeline *main_mau = this->get_pipeline ("main_control");
      main_mau->apply (packet.get ());
      packet->reset_exit ();

      bm::Deparser *deparser = this->get_deparser ("main_deparser");
      deparser->deparse (packet.get ());
      //   output_buffer.push_front (std::move (packet));
    }
}

int
P4PnaNic::ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                         const Address &destination)
{
  int len = packetIn->GetSize ();
  uint8_t *pkt_buffer = new uint8_t[len];
  packetIn->CopyData (pkt_buffer, len);

  // we limit the packet buffer to original size + 512 bytes, which means we
  // cannot add more than 512 bytes of header data to the packet, which should
  // be more than enough

  bm::PacketBuffer buffer (len + 512, (char *) pkt_buffer, len);

  std::unique_ptr<bm::Packet> bm_packet =
      new_packet_ptr (inPort, packet_id++, len, std::move (buffer));
  delete[] pkt_buffer;

  bm::PHV *phv = bm_packet->get_phv ();

  // many current p4 programs assume this
  // from pna spec - PNA does not mandate initialization of user-defined
  // metadata to known values as given as input to the parser
  phv->reset_metadata ();

  phv->get_field ("pna_main_parser_input_metadata.recirculated").set (0);
  phv->get_field ("pna_main_parser_input_metadata.input_port").set (inPort);

  // using packet register 0 to store length, this register will be updated for
  // each add_header / remove_header primitive call
  bm_packet->set_register (0, len);

  input_buffer.push_front (std::move (bm_packet));
  return 0;
}

int
P4PnaNic::receive_ (port_t port_num, const char *buffer, int len)
{
  NS_LOG_FUNCTION (this << " Port: " << port_num << " Len: " << len);
  return 0;
}

void
P4PnaNic::start_and_return_ ()
{
  NS_LOG_FUNCTION (this);
}
} // namespace ns3
