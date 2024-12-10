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
 * \TODO GetNanoSeconds or GetMicroSeconds?
 *
 */

#include "ns3/global.h"
#include "ns3/register_access.h"

#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/log.h"

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
#include <unordered_map>

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

bool
MirroringSessions::AddSession (int mirror_id, const MirroringSessionConfig &config)
{
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
MirroringSessions::DeleteSession (int mirror_id)
{
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
MirroringSessions::GetSession (int mirror_id, MirroringSessionConfig *config) const
{
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

// initialize static attributes

// Single mapping table, bm::Packet UID -> latest ns3::Packet UID
// For each new ns3::Packet created, it will have a new UID for the ns3::Packet, but we need to
// keep the same UID for the bm::Packet, so we need to keep a mapping between the ns3::Packet
// UID and bm::Packet UID, and after new ns3 packets creates, update the new value of
// ns3::Packet UID for mapping.
bm::packet_id_t P4Switch::packet_id = 0;
// static std::unordered_map<uint64_t, PacketInfo> uidMap;
// std::unordered_map<uint64_t, uint64_t> reverseUidMap;

int P4Switch::thrift_port = 9090;

P4Switch::P4Switch (BridgeP4NetDevice *netDevice)
{
  NS_LOG_FUNCTION (this);

  m_pNetDevice = netDevice;

  // @TODO port priority
  uint32_t nPorts = 4; // Default 4 ports
  uint32_t nPriorities = 8; // Default 8 priorities (3 bits)

  input_buffer = CreateObject<TwoTierP4Queue> ();
  input_buffer->Initialize ();

  queue_buffer = CreateObject<P4Queuebuffer> (nPorts, nPriorities);
  queue_buffer->Initialize ();
}

P4Switch::~P4Switch ()
{
  // @TODO delete all the objects
  input_buffer = nullptr;
  queue_buffer = nullptr;

  NS_LOG_INFO ("MyClass: Buffers destroyed");
}

TypeId
P4Switch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4Switch").SetParent<Object> ().SetGroupName ("Network");
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
  start_and_return ();

  std::this_thread::sleep_for (std::chrono::seconds (5));

  // Run the CLI commands to populate table entries
  std::string cmd = "run_bmv2_CLI --thrift_port " + std::to_string (port) + " " + commandsFile;
  std::system (cmd.c_str ());
}

int
P4Switch::receive_ (uint32_t port_num, const char *buffer, int len)
{
  // remove this
  return 0;
}

void
P4Switch::start_and_return_ ()
{
  NS_LOG_FUNCTION ("p4_switch has been start");
  check_queueing_metadata ();
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

int
P4Switch::ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                         const Address &destination)
{
  NS_LOG_FUNCTION (this);
  // @TODO Now Tag free, but we can add metadata for each pkt.

  Ptr<P4QueueItem> queue_item = Create<P4QueueItem> (packetIn, PacketType::NORMAL);
  input_buffer->Enqueue (queue_item);
  return 0;
}

void
P4Switch::push_input_buffer (std::unique_ptr<bm::Packet> &&bm_packet, PacketType packet_type)
{
  // Process the Re-submit, Re-circulate pkts, with high priority
  Ptr<P4QueueItem> queue_item =
      this->get_ns3_packet_queue_item (std::move (bm_packet), packet_type);

  if (input_buffer->Enqueue (queue_item))
    {
      NS_LOG_INFO ("Packet with type " << packet_type << " enqueued in P4 Input Buffer");
    }
  else
    {
      NS_LOG_WARN ("P4 Input Buffer is full, dropping packet");
    }
}

void
P4Switch::enqueue (uint32_t egress_port, std::unique_ptr<bm::Packet> &&bm_packet)
{
  NS_LOG_FUNCTION (this);
  bm_packet->set_egress_port (egress_port);

  bm::PHV *phv = bm_packet->get_phv ();

  size_t priority = 0; // default priority
  if (with_queueing_metadata)
    {
      phv->get_field ("queueing_metadata.enq_timestamp").set (Simulator::Now ().GetMicroSeconds ());

      priority = phv->has_field ("intrinsic_metadata.priority")
                     ? phv->get_field ("intrinsic_metadata.priority").get<size_t> ()
                     : 0u;
      if (priority >= nb_queues_per_port)
        {
          NS_LOG_ERROR ("Priority out of range, dropping packet");
          return;
        }
      phv->get_field ("queueing_metadata.enq_qdepth")
          .set (this->queue_buffer->GetVirtualQueueLengthPerPort (egress_port, priority));
    }

  Ptr<P4QueueItem> queue_item =
      this->get_ns3_packet_queue_item (std::move (bm_packet), PacketType::NORMAL);
  if (queue_buffer->Enqueue (queue_item))
    {
      NS_LOG_INFO ("Packet enqueued in P4QueueDisc, Port: " << egress_port
                                                            << ", Priority: " << priority);
    }
  else
    {
      NS_LOG_WARN ("QueueDisc P4QueueDisc is full, dropping packet, Port: "
                   << egress_port << ", Priority: " << priority);
    }
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

  Ptr<P4QueueItem> item = input_buffer->Dequeue ();
  if (item == nullptr)
    {
      NS_LOG_WARN ("P4 Input Buffer is empty, no packet to dequeue");
      return;
    }

  auto bm_packet = get_bm_packet (item);

  bm::Parser *parser = this->get_parser ("parser");
  bm::Pipeline *ingress_mau = this->get_pipeline ("ingress");
  bm::PHV *phv = bm_packet->get_phv ();

  uint32_t ingress_port = bm_packet->get_ingress_port ();
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
      NS_LOG_DEBUG ("Cloning packet at ingress, Packet ID: "
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

      push_input_buffer (std::move (bm_packet_copy), PacketType::RESUBMIT);
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

  enqueue (egress_port, std::move (bm_packet));
}

void
P4Switch::egress_deparser_processing ()
{
  NS_LOG_FUNCTION ("Dequeue packet from QueueBuffer");
  // Here need the ID.
  Ptr<P4QueueItem> item = queue_buffer->Dequeue ();
  if (item == nullptr)
    {
      NS_LOG_WARN ("GetQueueBuffer is empty, no packet to dequeue");
      return;
    }

  StandardMetadata *metadata = item->GetMetadata (); // use ptr
  uint32_t port = metadata->egress_port;

  auto bm_packet = get_bm_packet (item);

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

      size_t priority = phv->has_field ("intrinsic_metadata.priority")
                            ? phv->get_field ("intrinsic_metadata.priority").get<size_t> ()
                            : 0u;
      if (priority >= nb_queues_per_port)
        {
          NS_LOG_ERROR ("Priority out of range (nb_queues_per_port = " << nb_queues_per_port
                                                                       << "), dropping packet");
          return;
        }

      phv->get_field ("queueing_metadata.deq_qdepth")
          .set (this->queue_buffer->GetVirtualQueueLengthPerPort (port, priority));
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

      push_input_buffer (std::move (packet_copy), PacketType::RECIRCULATE);
      return;
    }

  // output_buffer.push_front(std::move(packet));
  // \TODO put pkts into the egress buffer with priority
}

std::unique_ptr<bm::Packet>
P4Switch::get_bm_packet (Ptr<P4QueueItem> item)
{
  StandardMetadata *metadata = item->GetMetadata ();
  uint16_t in_port = metadata->ns3_inport;
  uint64_t bmUid = metadata->bm_uid;

  // we limit the packet buffer to original size + 512 bytes, which means we
  // cannot add more than 512 bytes of header data to the packet, which should
  // be more than enough
  Ptr<Packet> ns_packet = item->GetPacket ();
  int len = ns_packet->GetSize ();
  uint8_t *pkt_buffer = new uint8_t[len];
  ns_packet->CopyData (pkt_buffer, len);
  bm::PacketBuffer buffer (len + 512, (char *) pkt_buffer, len);

  std::unique_ptr<bm::Packet> bm_packet = new_packet_ptr (in_port, bmUid, len, std::move (buffer));

  // fill the bm::packet with metadata
  metadata->WriteMetadataToBMPacket (std::move (bm_packet));
  delete[] pkt_buffer;

  return bm_packet;
}

std::unique_ptr<bm::Packet>
P4Switch::get_bm_packet_from_ingress (Ptr<Packet> ns_packet, uint16_t in_port)
{

  int len = ns_packet->GetSize ();
  uint8_t *pkt_buffer = new uint8_t[len];
  ns_packet->CopyData (pkt_buffer, len);
  bm::PacketBuffer buffer (len + 512, (char *) pkt_buffer, len);

  std::unique_ptr<bm::Packet> bm_packet =
      new_packet_ptr (in_port, packet_id++, len, std::move (buffer));
  delete[] pkt_buffer;

  // \TODO metadata

  return bm_packet;
}

Ptr<P4QueueItem>
P4Switch::get_ns3_packet_queue_item (std::unique_ptr<bm::Packet> bm_packet, PacketType packet_type)
{
  // Create a new ns3::Packet using the data buffer
  char *bm_buf = bm_packet.get ()->data ();
  size_t len = bm_packet.get ()->get_data_size ();
  Ptr<Packet> ns_packet = Create<Packet> ((uint8_t *) (bm_buf), len);

  Ptr<P4QueueItem> queue_item = Create<P4QueueItem> (ns_packet, packet_type);

  // fill the metadata in queue-item
  StandardMetadata *metadata = queue_item->GetMetadata ();
  metadata->GetMetadataFromBMPacket (std::move (bm_packet));

  return queue_item;
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
  // auto* phv = packet->get_phv();
  // auto& f_rid = phv->get_field("intrinsic_metadata.egress_rid");
  // const auto pre_out = pre->replicate({mgid});
  // auto packet_size = packet->get_register(RegisterAccess::PACKET_LENGTH_REG_IDX);

  // // \TODO using ns3 queue getNqueues to get the number of queues
  // for (const auto& out : pre_out)
  // {
  //     auto egress_port = out.egress_port;

  //     NS_LOG_DEBUG("Multicasting packet to egress port "
  //                  << egress_port << ", Packet ID: " << packet->get_packet_id()
  //                  << ", Size: " << packet->get_data_size() << " bytes");

  //     f_rid.set(out.rid);
  //     std::unique_ptr<bm::Packet> packet_copy = packet->clone_with_phv_ptr();
  //     RegisterAccess::clear_all(packet_copy.get());
  //     packet_copy->set_register(RegisterAccess::PACKET_LENGTH_REG_IDX, packet_size);
  //     enqueue(egress_port, std::move(packet_copy));
  // }
}

bool
P4Switch::mirroring_add_session (int mirror_id, const MirroringSessionConfig &config)
{
  return mirroring_sessions->AddSession (mirror_id, config);
}

bool
P4Switch::mirroring_delete_session (int mirror_id)
{
  return mirroring_sessions->DeleteSession (mirror_id);
}

bool
P4Switch::mirroring_get_session (int mirror_id, MirroringSessionConfig *config) const
{
  return mirroring_sessions->GetSession (mirror_id, config);
}

// int
// P4Switch::set_egress_priority_queue_depth(size_t port, size_t priority, const size_t depth_pkts)
// {
//     queue_buffer.set_capacity(port, priority, depth_pkts);
//     return 0;
// }

// int
// P4Switch::set_egress_queue_depth(size_t port, const size_t depth_pkts)
// {
//     queue_buffer.set_capacity(port, depth_pkts);
//     return 0;
// }

// int
// P4Switch::set_all_egress_queue_depths(const size_t depth_pkts)
// {
//     queue_buffer.set_capacity_for_all(depth_pkts);
//     return 0;
// }

// int
// P4Switch::set_egress_priority_queue_rate(size_t port, size_t priority, const uint64_t rate_pps)
// {
//     queue_buffer.set_rate(port, priority, rate_pps);
//     return 0;
// }

// int
// P4Switch::set_egress_queue_rate(size_t port, const uint64_t rate_pps)
// {
//     egress_buffers.set_rate(port, rate_pps);
//     return 0;
// }

// int
// P4Switch::set_all_egress_queue_rates(const uint64_t rate_pps)
// {
//     egress_buffers.set_rate_for_all(rate_pps);
//     return 0;
// }

} // namespace ns3
