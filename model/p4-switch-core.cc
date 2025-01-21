/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Stanford University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Stephen Ibanez <sibanez@stanford.edu>
 *          Mingyu Ma <mingyu.ma@tu-dresden.de>
 *
 *
 */

#include "ns3/global.h"
#include "ns3/register_access.h"

#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#include <bm/spdlog/spdlog.h>
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_DEBUG
#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/core/primitives.h>

NS_LOG_COMPONENT_DEFINE ("P4SwitchCore");

namespace ns3 {

namespace {

struct hash_ex
{
  uint32_t
  operator() (const char *buf, size_t s) const
  {
    const uint32_t p = 16777619;
    uint32_t hash = 2166136261;

    for (size_t i = 0; i < s; i++)
      hash = (hash ^ buf[i]) * p;

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return static_cast<uint32_t> (hash);
  }
};

struct bmv2_hash
{
  uint64_t
  operator() (const char *buf, size_t s) const
  {
    return bm::hash::xxh64 (buf, s);
  }
};

} // namespace

// if REGISTER_HASH calls placed in the anonymous namespace, some compiler can
// give an unused variable warning

REGISTER_HASH (hash_ex);
REGISTER_HASH (bmv2_hash);

class P4Switch::MirroringSessions
{
public:
  bool
  add_session (int mirror_id, const MirroringSessionConfig &config)
  {
    std::lock_guard<std::mutex> lock (mutex);
    if (0 <= mirror_id && mirror_id <= RegisterAccess::MAX_MIRROR_SESSION_ID)
      {
        sessions_map[mirror_id] = config;
        NS_LOG_INFO ("Session added with mirror_id=" << mirror_id);
        return true;
      }
    else
      {
        NS_LOG_ERROR ("mirror_id out of range. No session added.");
        return false;
      }
  }

  bool
  delete_session (int mirror_id)
  {
    std::lock_guard<std::mutex> lock (mutex);
    if (0 <= mirror_id && mirror_id <= RegisterAccess::MAX_MIRROR_SESSION_ID)
      {
        bool erased = sessions_map.erase (mirror_id) == 1;
        if (erased)
          {
            NS_LOG_INFO ("Session deleted with mirror_id=" << mirror_id);
          }
        else
          {
            NS_LOG_WARN ("No session found for mirror_id=" << mirror_id);
          }
        return erased;
      }
    else
      {
        NS_LOG_ERROR ("mirror_id out of range. No session deleted.");
        return false;
      }
  }

  bool
  get_session (int mirror_id, MirroringSessionConfig *config) const
  {
    std::lock_guard<std::mutex> lock (mutex);
    auto it = sessions_map.find (mirror_id);
    if (it == sessions_map.end ())
      {
        NS_LOG_WARN ("No session found for mirror_id=" << mirror_id);
        return false;
      }
    *config = it->second;
    NS_LOG_INFO ("Session retrieved for mirror_id=" << mirror_id);
    return true;
  }

private:
  // using Mutex = std::mutex;
  // using Lock = std::lock_guard<Mutex>;

  mutable std::mutex mutex;
  std::unordered_map<int, MirroringSessionConfig> sessions_map;
};

bm::packet_id_t P4Switch::packet_id = 0;
// int P4Switch::thrift_port = 9090;

P4Switch::P4Switch (BridgeP4NetDevice *netDevice, bool enable_swap, port_t drop_port,
                    size_t nb_queues_per_port)
    : Switch (enable_swap),
      drop_port (drop_port),
      input_buffer (
          new InputBuffer (10240 /* normal capacity */, 1024 /* resubmit/recirc capacity */)),
      nb_queues_per_port (nb_queues_per_port),
      egress_buffers (nb_egress_threads, 10240, EgressThreadMapper (nb_egress_threads),
                      nb_queues_per_port),
      output_buffer (10240),
      // cannot use std::bind because of a clang bug
      // https://stackoverflow.com/questions/32030141/is-this-incorrect-use-of-stdbind-or-a-compiler-bug
      my_transmit_fn ([this] (port_t port_num, uint64_t pkt_id, const char *buffer, int len) {
        _BM_UNUSED (pkt_id);
        this->transmit_fn (port_num, buffer, len);
      }),
      m_pre (new bm::McSimplePreLAG ()),
      start (std::chrono::high_resolution_clock::now ()),
      mirroring_sessions (new MirroringSessions ())

{
  NS_LOG_FUNCTION (this);

  add_component<bm::McSimplePreLAG> (m_pre);

  add_required_field ("standard_metadata", "ingress_port");
  add_required_field ("standard_metadata", "packet_length");
  add_required_field ("standard_metadata", "instance_type");
  add_required_field ("standard_metadata", "egress_spec");
  add_required_field ("standard_metadata", "egress_port");

  force_arith_header ("standard_metadata");
  force_arith_header ("queueing_metadata");
  force_arith_header ("intrinsic_metadata");

  // import_primitives (); // just remove

  static int switch_id = 1;
  p4_switch_ID = switch_id++;
  NS_LOG_INFO ("Init P4 Switch ID: " << p4_switch_ID);

  m_pNetDevice = netDevice;

  // init the scheduler with [pps] packets per second
  worker_id = 0;

  // m_ingressTimerEvent = EventId (); // default initial value
  m_egressTimerEvent = EventId (); // default initial value
  // m_transmitTimerEvent = EventId (); // default initial value

  egress_buffers.set_rate_for_all (1000000 / P4GlobalVar::g_switchBottleNeck);
  NS_LOG_DEBUG ("Queue rate set to " << 1000000 / P4GlobalVar::g_switchBottleNeck << " pps");

  std::string time_ref_fast = std::to_string (P4GlobalVar::g_switchBottleNeck / 2) + "us";
  std::string time_ref_bottle_neck = std::to_string (P4GlobalVar::g_switchBottleNeck) + "us";
  NS_LOG_INFO ("Time reference for ingress: " << time_ref_fast
                                              << ", for egress: " << time_ref_bottle_neck);
  // m_ingressTimeReference = Time (time_ref_fast);
  m_egressTimeReference = Time (time_ref_bottle_neck);
  // m_transmitTimeReference = Time (time_ref_fast);

  // Now init the switch queue with 0 port, later the bridge will add the ports
  uint32_t nPorts = netDevice->GetNBridgePorts ();
  uint32_t nPriorities = 8; // Default 8 priorities (3 bits)
  NS_LOG_INFO ("Init the switch queue with " << nPorts << " ports and " << nPriorities
                                             << " priorities");
}

P4Switch::~P4Switch ()
{
  input_buffer->push_front (InputBuffer::PacketType::SENTINEL, nullptr);
  for (size_t i = 0; i < nb_egress_threads; i++)
    {
      // The push_front call is called inside a while loop because there is no
      // guarantee that the sentinel was enqueued otherwise. It should not be an
      // issue because at this stage the ingress thread has been sent a signal to
      // stop, and only egress clones can be sent to the buffer.
      while (egress_buffers.push_front (i, 0, nullptr) == 0)
        continue;
    }
  output_buffer.push_front (nullptr);
}

TypeId
P4Switch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4Switch").SetParent<Object> ().SetGroupName ("P4sim");
  return tid;
}

int
P4Switch::InitFromCommandLineOptionsLocal (int argc, char *argv[])
{
  bm::OptionsParser parser;
  parser.parse (argc, argv, m_argParser);

  // create a dummy transport
  std::shared_ptr<bm::TransportIface> transport =
      std::shared_ptr<bm::TransportIface> (bm::TransportIface::make_dummy ());

  int status = 0;
  if (parser.no_p4)
    // with out p4-json, acctually the switch will wait for the configuration(p4-json) before
    // work
    status = init_objects_empty (parser.device_id, transport);
  else
    // load p4 configuration files xxxx.json to switch
    status = init_objects (parser.config_file_path, parser.device_id, transport);
  return status;
}

void
P4Switch::run_cli (std::string commandsFile)
{
  int port = get_runtime_port ();
  bm_runtime::start_server (this, port);
  // start_and_return ();

  std::this_thread::sleep_for (std::chrono::seconds (3));

  // Run the CLI commands to populate table entries
  std::string cmd = "run_bmv2_CLI --thrift_port " + std::to_string (port) + " " + commandsFile;
  std::system (cmd.c_str ());
}

int
P4Switch::receive_ (uint32_t port_num, const char *buffer, int len)
{
  // check ReceivePacket()
  return 0;
}

void
P4Switch::start_and_return_ ()
{
  NS_LOG_FUNCTION ("p4_switch has been start");
  check_queueing_metadata ();

  if (!m_egressTimeReference.IsZero ())
    {
      NS_LOG_INFO ("Scheduling initial timer event using m_egressTimeReference = "
                   << m_egressTimeReference.GetNanoSeconds () << " ns");
      m_egressTimerEvent =
          Simulator::Schedule (m_egressTimeReference, &P4Switch::RunEgressTimerEvent, this);
    }
}

void
P4Switch::RunIngressTimerEvent ()
{
  NS_LOG_FUNCTION ("p4_switch has been triggered by the ingress timer event");
}

void
P4Switch::RunEgressTimerEvent ()
{
  NS_LOG_FUNCTION ("p4_switch has been triggered by the egress timer event");
  egress_deparser_processing (worker_id);
  m_egressTimerEvent =
      Simulator::Schedule (m_egressTimeReference, &P4Switch::RunEgressTimerEvent, this);
}

void
P4Switch::swap_notify_ ()
{
  NS_LOG_FUNCTION ("p4_switch has been notified of a config swap");
  check_queueing_metadata ();
}

void
P4Switch::check_queueing_metadata ()
{
  // TODO(antonin): add qid in required fields
  bool enq_timestamp_e = field_exists ("queueing_metadata", "enq_timestamp");
  bool enq_qdepth_e = field_exists ("queueing_metadata", "enq_qdepth");
  bool deq_timedelta_e = field_exists ("queueing_metadata", "deq_timedelta");
  bool deq_qdepth_e = field_exists ("queueing_metadata", "deq_qdepth");
  if (enq_timestamp_e || enq_qdepth_e || deq_timedelta_e || deq_qdepth_e)
    {
      if (enq_timestamp_e && enq_qdepth_e && deq_timedelta_e && deq_qdepth_e)
        {
          with_queueing_metadata = true;
          return;
        }
      else
        {
          NS_LOG_WARN ("Your JSON input defines some but not all queueing metadata fields");
        }
    }
  else
    {
      NS_LOG_WARN ("Your JSON input does not define any queueing metadata fields");
    }
  with_queueing_metadata = false;
}

void
P4Switch::reset_target_state_ ()
{
  NS_LOG_DEBUG ("Resetting simple_switch target-specific state");
  get_component<bm::McSimplePreLAG> ()->reset_state ();
}

int
P4Switch::ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                         const Address &destination)
{
  NS_LOG_FUNCTION (this);

  int len = packetIn->GetSize ();
  uint8_t *pkt_buffer = new uint8_t[len];
  packetIn->CopyData (pkt_buffer, len);
  bm::PacketBuffer buffer (len + 512, (char *) pkt_buffer, len);

  std::unique_ptr<bm::Packet> bm_packet =
      new_packet_ptr (inPort, packet_id++, len, std::move (buffer));
  delete[] pkt_buffer;

  bm::PHV *phv = bm_packet->get_phv ();
  len = bm_packet.get ()->get_data_size ();
  bm_packet.get ()->set_ingress_port (inPort);

  phv->reset_metadata ();

  // reset, set ns3 protocol and address
  RegisterAccess::clear_all (bm_packet.get ());
  RegisterAccess::set_ns_protocol (bm_packet.get (), protocol);

  int addr_index = GetAddressIndex (destination);
  RegisterAccess::set_ns_address (bm_packet.get (), addr_index);

  // setting standard metadata
  phv->get_field ("standard_metadata.ingress_port").set (inPort);

  // using packet register 0 to store length, this register will be updated for
  // each add_header / remove_header primitive call
  bm_packet->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, len);
  phv->get_field ("standard_metadata.packet_length").set (len);
  bm::Field &f_instance_type = phv->get_field ("standard_metadata.instance_type");
  f_instance_type.set (PKT_INSTANCE_TYPE_NORMAL);
  if (phv->has_field ("intrinsic_metadata.ingress_global_timestamp"))
    {
      phv->get_field ("intrinsic_metadata.ingress_global_timestamp")
          .set (Simulator::Now ().GetMicroSeconds ());
    }

  input_buffer->push_front (InputBuffer::PacketType::NORMAL, std::move (bm_packet));

  parser_ingress_processing ();
  return 0;
}

void
P4Switch::enqueue (port_t egress_port, std::unique_ptr<bm::Packet> &&packet)
{
  packet->set_egress_port (egress_port);

  bm::PHV *phv = packet->get_phv ();

  if (with_queueing_metadata)
    {
      uint64_t enq_time_stamp = Simulator::Now ().GetMicroSeconds ();
      phv->get_field ("queueing_metadata.enq_timestamp").set (enq_time_stamp);
      phv->get_field ("queueing_metadata.enq_qdepth").set (egress_buffers.size (egress_port));
    }

  size_t priority = phv->has_field (SSWITCH_PRIORITY_QUEUEING_SRC)
                        ? phv->get_field (SSWITCH_PRIORITY_QUEUEING_SRC).get<size_t> ()
                        : 0u;
  if (priority >= nb_queues_per_port)
    {
      bm::Logger::get ()->error ("Priority out of range, dropping packet");
      return;
    }

  egress_buffers.push_front (egress_port, nb_queues_per_port - 1 - priority, std::move (packet));

  NS_LOG_INFO ("Packet enqueued in P4QueueDisc, Port: " << egress_port
                                                        << ", Priority: " << priority);
}

void
P4Switch::push_transmit_buffer (std::unique_ptr<bm::Packet> &&bm_packet)
{
  NS_LOG_FUNCTION (this);
}

void
P4Switch::parser_ingress_processing ()
{
  NS_LOG_FUNCTION (this);

  // Ptr<P4QueueItem> item = input_buffer->Dequeue ();
  // if (item == nullptr)
  //   {
  //     // NS_LOG_WARN ("P4 Input Buffer is empty, no packet to dequeue");
  //     return;
  //   }

  // std::unique_ptr<bm::Packet> bm_packet = item->GetPacket ();

  std::unique_ptr<bm::Packet> bm_packet;
  input_buffer->pop_back (&bm_packet);
  if (bm_packet == nullptr)
    return;

  bm::Parser *parser = this->get_parser ("parser");
  bm::Pipeline *ingress_mau = this->get_pipeline ("ingress");
  bm::PHV *phv = bm_packet->get_phv ();

  uint32_t ingress_port = bm_packet->get_ingress_port ();

  NS_LOG_INFO ("Processing packet from port "
               << ingress_port << ", Packet ID: " << bm_packet->get_packet_id ()
               << ", Size: " << bm_packet->get_data_size () << " bytes");

  auto ingress_packet_size = bm_packet->get_register (RegisterAccess::PACKET_LENGTH_REG_IDX);

  /* This looks like it comes out of the blue. However this is needed for
       ingress cloning. The parser updates the buffer state (pops the parsed
       headers) to make the deparser's job easier (the same buffer is
       re-used). But for ingress cloning, the original packet is needed. This
       kind of looks hacky though. Maybe a better solution would be to have the
       parser leave the buffer unchanged, and move the pop logic to the
       deparser. TODO? */
  const bm::Packet::buffer_state_t packet_in_state = bm_packet->save_buffer_state ();

  parser->parse (bm_packet.get ());

  if (phv->has_field ("standard_metadata.parser_error"))
    {
      phv->get_field ("standard_metadata.parser_error").set (bm_packet->get_error_code ().get ());
    }
  if (phv->has_field ("standard_metadata.checksum_error"))
    {
      phv->get_field ("standard_metadata.checksum_error")
          .set (bm_packet->get_checksum_error () ? 1 : 0);
    }

  ingress_mau->apply (bm_packet.get ());

  bm_packet->reset_exit ();

  bm::Field &f_egress_spec = phv->get_field ("standard_metadata.egress_spec");
  uint32_t egress_spec = f_egress_spec.get_uint ();

  auto clone_mirror_session_id = RegisterAccess::get_clone_mirror_session_id (bm_packet.get ());
  auto clone_field_list = RegisterAccess::get_clone_field_list (bm_packet.get ());

  int learn_id = RegisterAccess::get_lf_field_list (bm_packet.get ());
  unsigned int mgid = 0u;

  // detect mcast support, if this is true we assume that other fields needed
  // for mcast are also defined
  if (phv->has_field ("intrinsic_metadata.mcast_grp"))
    {
      bm::Field &f_mgid = phv->get_field ("intrinsic_metadata.mcast_grp");
      mgid = f_mgid.get_uint ();
    }

  // INGRESS CLONING
  if (clone_mirror_session_id)
    {
      NS_LOG_INFO ("Cloning packet at ingress, Packet ID: "
                   << bm_packet->get_packet_id () << ", Size: " << bm_packet->get_data_size ()
                   << " bytes");

      RegisterAccess::set_clone_mirror_session_id (bm_packet.get (), 0);
      RegisterAccess::set_clone_field_list (bm_packet.get (), 0);
      MirroringSessionConfig config;
      // Extract the part of clone_mirror_session_id that contains the
      // actual session id.
      clone_mirror_session_id &= RegisterAccess::MIRROR_SESSION_ID_MASK;
      bool is_session_configured =
          mirroring_get_session (static_cast<int> (clone_mirror_session_id), &config);
      if (is_session_configured)
        {
          const bm::Packet::buffer_state_t packet_out_state = bm_packet->save_buffer_state ();
          bm_packet->restore_buffer_state (packet_in_state);
          int field_list_id = clone_field_list;
          std::unique_ptr<bm::Packet> bm_packet_copy = bm_packet->clone_no_phv_ptr ();
          RegisterAccess::clear_all (bm_packet_copy.get ());
          bm_packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, ingress_packet_size);

          // We need to parse again.
          // The alternative would be to pay the (huge) price of PHV copy for
          // every ingress packet.
          // Since parsers can branch on the ingress port, we need to preserve it
          // to ensure re-parsing gives the same result as the original parse.
          // TODO(https://github.com/p4lang/behavioral-model/issues/795): other
          // standard metadata should be preserved as well.
          bm_packet_copy->get_phv ()
              ->get_field ("standard_metadata.ingress_port")
              .set (ingress_port);
          parser->parse (bm_packet_copy.get ());
          copy_field_list_and_set_type (bm_packet, bm_packet_copy, PKT_INSTANCE_TYPE_INGRESS_CLONE,
                                        field_list_id);
          if (config.mgid_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to MGID {}" << config.mgid);
              multicast (bm_packet_copy.get (), config.mgid);
            }
          if (config.egress_port_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to egress port "
                            << config.egress_port << ", Packet ID: " << bm_packet->get_packet_id ()
                            << ", Size: " << bm_packet->get_data_size () << " bytes");
              enqueue (config.egress_port, std::move (bm_packet_copy));
              // Ptr<P4QueueItem> item_clone =
              //     Create<P4QueueItem> (std::move (bm_packet_copy), PacketType::NORMAL);
              // enqueue (config.egress_port, item_clone);
            }
          bm_packet->restore_buffer_state (packet_out_state);
        }
    }

  // LEARNING
  if (learn_id > 0)
    {
      get_learn_engine ()->learn (learn_id, *bm_packet.get ());
    }

  // RESUBMIT
  auto resubmit_flag = RegisterAccess::get_resubmit_flag (bm_packet.get ());
  if (resubmit_flag)
    {
      NS_LOG_DEBUG ("Resubmitting packet");

      // get the packet ready for being parsed again at the beginning of
      // ingress
      bm_packet->restore_buffer_state (packet_in_state);
      int field_list_id = resubmit_flag;
      RegisterAccess::set_resubmit_flag (bm_packet.get (), 0);
      // TODO(antonin): a copy is not needed here, but I don't yet have an
      // optimized way of doing this
      std::unique_ptr<bm::Packet> bm_packet_copy = bm_packet->clone_no_phv_ptr ();
      bm::PHV *phv_copy = bm_packet_copy->get_phv ();
      copy_field_list_and_set_type (bm_packet, bm_packet_copy, PKT_INSTANCE_TYPE_RESUBMIT,
                                    field_list_id);
      RegisterAccess::clear_all (bm_packet_copy.get ());
      bm_packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, ingress_packet_size);
      phv_copy->get_field ("standard_metadata.packet_length").set (ingress_packet_size);

      // push_input_buffer (std::move (bm_packet_copy), PacketType::RESUBMIT);
      input_buffer->push_front (InputBuffer::PacketType::RESUBMIT, std::move (bm_packet_copy));
      return;
    }

  // MULTICAST
  if (mgid != 0)
    {
      NS_LOG_DEBUG ("Multicast requested for packet");
      auto &f_instance_type = phv->get_field ("standard_metadata.instance_type");
      f_instance_type.set (PKT_INSTANCE_TYPE_REPLICATION);
      multicast (bm_packet.get (), mgid);
      // when doing multicast, we discard the original packet
      return;
    }

  uint32_t egress_port = egress_spec;
  NS_LOG_DEBUG ("Egress port is " << egress_port);

  if (egress_port == default_drop_port)
    {
      // drop packet
      NS_LOG_DEBUG ("Dropping packet at the end of ingress");
      return;
    }
  auto &f_instance_type = phv->get_field ("standard_metadata.instance_type");
  f_instance_type.set (PKT_INSTANCE_TYPE_NORMAL);

  // item->SetPacket(std::move (bm_packet)); // move ownership to item
  // enqueue (egress_port, item);
  enqueue (egress_port, std::move (bm_packet));
}

void
P4Switch::egress_deparser_processing (size_t worker_id)
{
  NS_LOG_FUNCTION ("Dequeue packet from QueueBuffer");
  std::unique_ptr<bm::Packet> bm_packet;
  size_t port;
  size_t priority;

  int queue_number = default_nb_queues_per_port;

  for (int i = 0; i < queue_number; i++)
    {
      if (egress_buffers.size (i) > 0)
        {
          break;
        }
      if (i == queue_number - 1)
        {
          return;
        }
    }

  egress_buffers.pop_back (worker_id, &port, &priority, &bm_packet);
  if (bm_packet == nullptr)
    return;

  // Ptr<P4QueueItem> item = queue_buffer->Dequeue ();
  // if (item == nullptr)
  //   {
  //     // NS_LOG_WARN ("GetQueueBuffer is empty, no packet to dequeue");
  //     return;
  //   }

  // StandardMetadata *metadata = item->GetMetadata (); // use ptr

  // this information is done before egress processing
  // uint32_t port = metadata->egress_port;
  // uint16_t protocol = metadata->ns3_protocol;
  // Address destination = metadata->ns3_destination;

  // std::unique_ptr bm_packet = item->GetPacket ();

  NS_LOG_FUNCTION ("Egress processing for the packet");
  bm::PHV *phv = bm_packet->get_phv ();
  bm::Pipeline *egress_mau = this->get_pipeline ("egress");
  bm::Deparser *deparser = this->get_deparser ("deparser");

  if (phv->has_field ("intrinsic_metadata.egress_global_timestamp"))
    {
      phv->get_field ("intrinsic_metadata.egress_global_timestamp")
          .set (Simulator::Now ().GetMicroSeconds ());
    }

  if (with_queueing_metadata)
    {
      uint64_t enq_timestamp = phv->get_field ("queueing_metadata.enq_timestamp").get<uint64_t> ();
      uint64_t now = Simulator::Now ().GetMicroSeconds ();
      phv->get_field ("queueing_metadata.deq_timedelta").set (now - enq_timestamp);

      size_t priority = phv->has_field (SSWITCH_PRIORITY_QUEUEING_SRC)
                            ? phv->get_field (SSWITCH_PRIORITY_QUEUEING_SRC).get<size_t> ()
                            : 0u;
      if (priority >= nb_queues_per_port)
        {
          NS_LOG_ERROR ("Priority out of range (nb_queues_per_port = " << nb_queues_per_port
                                                                       << "), dropping packet");
          return;
        }

      phv->get_field ("queueing_metadata.deq_qdepth").set (egress_buffers.size (port));
      if (phv->has_field ("queueing_metadata.qid"))
        {
          auto &qid_f = phv->get_field ("queueing_metadata.qid");
          qid_f.set (nb_queues_per_port - 1 - priority);
        }
    }

  phv->get_field ("standard_metadata.egress_port").set (port);

  bm::Field &f_egress_spec = phv->get_field ("standard_metadata.egress_spec");
  f_egress_spec.set (0);

  phv->get_field ("standard_metadata.packet_length")
      .set (bm_packet->get_register (RegisterAccess::PACKET_LENGTH_REG_IDX));

  egress_mau->apply (bm_packet.get ());

  auto clone_mirror_session_id = RegisterAccess::get_clone_mirror_session_id (bm_packet.get ());
  auto clone_field_list = RegisterAccess::get_clone_field_list (bm_packet.get ());

  // EGRESS CLONING
  if (clone_mirror_session_id)
    {
      NS_LOG_DEBUG ("Cloning packet at egress, Packet ID: "
                    << bm_packet->get_packet_id () << ", Size: " << bm_packet->get_data_size ()
                    << " bytes");
      RegisterAccess::set_clone_mirror_session_id (bm_packet.get (), 0);
      RegisterAccess::set_clone_field_list (bm_packet.get (), 0);
      MirroringSessionConfig config;
      // Extract the part of clone_mirror_session_id that contains the
      // actual session id.
      clone_mirror_session_id &= RegisterAccess::MIRROR_SESSION_ID_MASK;
      bool is_session_configured =
          mirroring_get_session (static_cast<int> (clone_mirror_session_id), &config);
      if (is_session_configured)
        {
          int field_list_id = clone_field_list;
          std::unique_ptr<bm::Packet> packet_copy = bm_packet->clone_with_phv_reset_metadata_ptr ();
          bm::PHV *phv_copy = packet_copy->get_phv ();
          bm::FieldList *field_list = this->get_field_list (field_list_id);
          field_list->copy_fields_between_phvs (phv_copy, phv);
          phv_copy->get_field ("standard_metadata.instance_type")
              .set (PKT_INSTANCE_TYPE_EGRESS_CLONE);
          auto packet_size = bm_packet->get_register (RegisterAccess::PACKET_LENGTH_REG_IDX);
          RegisterAccess::clear_all (packet_copy.get ());
          packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, packet_size);
          if (config.mgid_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to MGID " << config.mgid);
              multicast (packet_copy.get (), config.mgid);
            }
          if (config.egress_port_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to egress port " << config.egress_port);
              // TODO This create a copy packet(new bm packet), but the UID mapping may need to
              // be updated in map.
              // Ptr<P4QueueItem> item_clone =
              //     Create<P4QueueItem> (std::move (packet_copy), PacketType::NORMAL);
              // enqueue (config.egress_port, item_clone);
              enqueue (config.egress_port, std::move (packet_copy));
            }
        }
    }

  // TODO(antonin): should not be done like this in egress pipeline
  uint32_t egress_spec = f_egress_spec.get_uint ();
  if (egress_spec == default_drop_port)
    {
      // drop packet
      NS_LOG_DEBUG ("Dropping packet at the end of egress");
      return;
    }

  deparser->deparse (bm_packet.get ());

  // RECIRCULATE
  auto recirculate_flag = RegisterAccess::get_recirculate_flag (bm_packet.get ());
  if (recirculate_flag)
    {
      NS_LOG_DEBUG ("Recirculating packet");

      int field_list_id = recirculate_flag;
      RegisterAccess::set_recirculate_flag (bm_packet.get (), 0);
      bm::FieldList *field_list = this->get_field_list (field_list_id);
      // TODO(antonin): just like for resubmit, there is no need for a copy
      // here, but it is more convenient for this first prototype
      std::unique_ptr<bm::Packet> packet_copy = bm_packet->clone_no_phv_ptr ();
      bm::PHV *phv_copy = packet_copy->get_phv ();
      phv_copy->reset_metadata ();
      field_list->copy_fields_between_phvs (phv_copy, phv);
      phv_copy->get_field ("standard_metadata.instance_type").set (PKT_INSTANCE_TYPE_RECIRC);
      size_t packet_size = packet_copy->get_data_size ();
      RegisterAccess::clear_all (packet_copy.get ());
      packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, packet_size);
      phv_copy->get_field ("standard_metadata.packet_length").set (packet_size);
      // TODO(antonin): really it may be better to create a new packet here or
      // to fold this functionality into the Packet class?
      packet_copy->set_ingress_length (packet_size);
      input_buffer->push_front (InputBuffer::PacketType::RECIRCULATE, std::move (packet_copy));
      return;
    }

  uint16_t protocol = RegisterAccess::get_ns_protocol (bm_packet.get ());
  int addr_index = RegisterAccess::get_ns_address (bm_packet.get ());

  Ptr<Packet> ns_packet = this->get_ns3_packet (std::move (bm_packet));
  m_pNetDevice->SendNs3Packet (ns_packet, port, protocol, destination_list[addr_index]);
}

Ptr<Packet>
P4Switch::get_ns3_packet (std::unique_ptr<bm::Packet> &&bm_packet)
{
  // Create a new ns3::Packet using the data buffer
  char *bm_buf = bm_packet.get ()->data ();
  size_t len = bm_packet.get ()->get_data_size ();
  Ptr<Packet> ns_packet = Create<Packet> ((uint8_t *) (bm_buf), len);

  return ns_packet;
}

void
P4Switch::copy_field_list_and_set_type (const std::unique_ptr<bm::Packet> &packet,
                                        const std::unique_ptr<bm::Packet> &packet_copy,
                                        PktInstanceType copy_type, int field_list_id)
{
  bm::PHV *phv_copy = packet_copy->get_phv ();
  phv_copy->reset_metadata ();
  bm::FieldList *field_list = this->get_field_list (field_list_id);
  field_list->copy_fields_between_phvs (phv_copy, packet->get_phv ());
  phv_copy->get_field ("standard_metadata.instance_type").set (copy_type);
}

void
P4Switch::multicast (bm::Packet *packet, unsigned int mgid)
{
  NS_LOG_FUNCTION (this);
  auto *phv = packet->get_phv ();
  auto &f_rid = phv->get_field ("intrinsic_metadata.egress_rid");
  const auto pre_out = m_pre->replicate ({mgid});
  auto packet_size = packet->get_register (RegisterAccess::PACKET_LENGTH_REG_IDX);
  for (const auto &out : pre_out)
    {
      auto egress_port = out.egress_port;
      NS_LOG_DEBUG ("Replicating packet on port " << egress_port);
      f_rid.set (out.rid);
      std::unique_ptr<bm::Packet> packet_copy = packet->clone_with_phv_ptr ();
      RegisterAccess::clear_all (packet_copy.get ());
      packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, packet_size);
      enqueue (egress_port, std::move (packet_copy));
    }
}

bool
P4Switch::mirroring_add_session (int mirror_id, const MirroringSessionConfig &config)
{
  return mirroring_sessions->add_session (mirror_id, config);
}

bool
P4Switch::mirroring_delete_session (int mirror_id)
{
  return mirroring_sessions->delete_session (mirror_id);
}

bool
P4Switch::mirroring_get_session (int mirror_id, MirroringSessionConfig *config) const
{
  return mirroring_sessions->get_session (mirror_id, config);
}

int
P4Switch::set_egress_priority_queue_depth (size_t port, size_t priority, const size_t depth_pkts)
{
  egress_buffers.set_capacity (port, priority, depth_pkts);
  return 0;
}

int
P4Switch::set_egress_queue_depth (size_t port, const size_t depth_pkts)
{
  egress_buffers.set_capacity (port, depth_pkts);
  return 0;
}

int
P4Switch::set_all_egress_queue_depths (const size_t depth_pkts)
{
  egress_buffers.set_capacity_for_all (depth_pkts);
  return 0;
}

int
P4Switch::set_egress_priority_queue_rate (size_t port, size_t priority, const uint64_t rate_pps)
{
  egress_buffers.set_rate (port, priority, rate_pps);
  return 0;
}

int
P4Switch::set_egress_queue_rate (size_t port, const uint64_t rate_pps)
{
  egress_buffers.set_rate (port, rate_pps);
  return 0;
}

int
P4Switch::set_all_egress_queue_rates (const uint64_t rate_pps)
{
  egress_buffers.set_rate_for_all (rate_pps);
  return 0;
}

int
P4Switch::GetAddressIndex (const Address &destination)
{
  auto it = address_map.find (destination);
  if (it != address_map.end ())
    {
      // Address already exists, return its index
      return it->second;
    }
  else
    {
      // Address does not exist, add it to the list and map
      int new_index = destination_list.size ();
      destination_list.push_back (destination);
      address_map[destination] = new_index;
      return new_index;
    }
}
} // namespace ns3
