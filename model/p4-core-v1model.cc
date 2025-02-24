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

#include "ns3/register_access.h"
#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/p4-core-v1model.h"

#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/tables.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/core/primitives.h>

NS_LOG_COMPONENT_DEFINE ("P4CoreV1model");

namespace ns3 {

namespace {

struct hash_ex_v1model
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

struct bmv2_hash_v1model
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
REGISTER_HASH (hash_ex_v1model);
REGISTER_HASH (bmv2_hash_v1model);

class P4CoreV1model::MirroringSessions
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
  mutable std::mutex mutex;
  std::unordered_map<int, MirroringSessionConfig> sessions_map;
};

bm::packet_id_t P4CoreV1model::packet_id = 0;

P4CoreV1model::P4CoreV1model (P4SwitchNetDevice *net_device, bool enable_swap, uint64_t packet_rate,
                              size_t input_buffer_size_low, size_t input_buffer_size_high,
                              size_t queue_buffer_size, port_t drop_port, size_t nb_queues_per_port)
    : bm::Switch (enable_swap),
      drop_port (drop_port),
      nb_queues_per_port (nb_queues_per_port),
      packet_rate_pps (packet_rate),
      start_timestamp (Simulator::Now ().GetNanoSeconds ()),
      input_buffer (new InputBuffer (input_buffer_size_low /* normal capacity */,
                                     input_buffer_size_high /* resubmit/recirc capacity */)),
      egress_buffer (nb_egress_threads, queue_buffer_size, EgressThreadMapper (nb_egress_threads),
                     nb_queues_per_port),
      output_buffer (SSWITCH_OUTPUT_BUFFER_SIZE),
      m_pre (new bm::McSimplePreLAG ()),
      mirroring_sessions (new MirroringSessions ())

{
  NS_LOG_FUNCTION (this << " Switch ID Drop port: " << drop_port
                        << " Queues per port: " << nb_queues_per_port);
  NS_LOG_DEBUG ("Creating P4CoreV1model with drop port "
                << drop_port << " and " << nb_queues_per_port << " queues per port");

  add_component<bm::McSimplePreLAG> (m_pre);

  add_required_field ("standard_metadata", "ingress_port");
  add_required_field ("standard_metadata", "packet_length");
  add_required_field ("standard_metadata", "instance_type");
  add_required_field ("standard_metadata", "egress_spec");
  add_required_field ("standard_metadata", "egress_port");

  force_arith_header ("standard_metadata");
  force_arith_header ("queueing_metadata");
  force_arith_header ("intrinsic_metadata");

  static int switch_id = 1;
  p4_switch_ID = switch_id++;
  NS_LOG_INFO ("Init P4 Switch with ID: " << p4_switch_ID);

  bridge_net_device = net_device;

  CalculateScheduleTime ();
}

P4CoreV1model::~P4CoreV1model ()
{
  input_buffer->push_front (InputBuffer::PacketType::SENTINEL, nullptr);
  for (size_t i = 0; i < nb_egress_threads; i++)
    {
      // The push_front call is called inside a while loop because there is no
      // guarantee that the sentinel was enqueued otherwise. It should not be an
      // issue because at this stage the ingress thread has been sent a signal to
      // stop, and only egress clones can be sent to the buffer.
      while (egress_buffer.push_front (i, 0, nullptr) == 0)
        continue;
    }
  output_buffer.push_front (nullptr);
}

void
P4CoreV1model::InitSwitchWithP4 (std::string jsonPath, std::string flowTablePath)
{
  NS_LOG_FUNCTION (this); // Log function entry

  int status = 0; // Status flag for initialization

  /**
   * @brief NS3PIFOTM mode initializes the switch using a JSON file in jsonPath
   * and populates the flow table entry in flowTablePath.
   */
  NS_LOG_INFO ("Initializing P4CoreV1model with NS3PIFOTM mode.");

  static int p4_switch_ctrl_plane_thrift_port = 9090;

  bm::OptionsParser opt_parser;
  opt_parser.config_file_path = jsonPath;
  opt_parser.debugger_addr =
      "ipc:///tmp/bmv2-" + std::to_string (p4_switch_ctrl_plane_thrift_port) + "-debug.ipc";
  opt_parser.notifications_addr =
      "ipc:///tmp/bmv2-" + std::to_string (p4_switch_ctrl_plane_thrift_port) + "-notifications.ipc";
  opt_parser.file_logger =
      "/tmp/bmv2-" + std::to_string (p4_switch_ctrl_plane_thrift_port) + "-pipeline.log";
  opt_parser.thrift_port = p4_switch_ctrl_plane_thrift_port++;
  opt_parser.console_logging = true;

  // Initialize the switch
  status = 0;
  status = init_from_options_parser (opt_parser);
  if (status != 0)
    {
      NS_LOG_ERROR ("Failed to initialize P4CoreV1model with NS3PIFOTM mode.");
      return; // Avoid exiting simulation
    }

  // Start the runtime server
  int port = get_runtime_port ();
  bm_runtime::start_server (this, port);

  // Execute CLI command to populate the flow table
  std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string (port) + " < " +
                    flowTablePath + " > /dev/null 2>&1";

  int result = std::system (cmd.c_str ());

  // sleep for 2 second to avoid the server not ready
  sleep (2);
  if (result != 0)
    {
      NS_LOG_ERROR ("Error executing flow table population command: " << cmd);
    }

  // Note: Consider stopping the server if needed
  // bm_runtime::stop_server();

  if (status != 0)
    {
      NS_LOG_ERROR ("P4CoreV1model initialization failed with status: " << status);
      return;
    }

  NS_LOG_INFO ("P4CoreV1model initialization completed successfully.");
}

int
P4CoreV1model::InitFromCommandLineOptions (int argc, char *argv[])
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
P4CoreV1model::RunCli (const std::string &commandsFile)
{
  NS_LOG_FUNCTION (this << " Switch ID: " << p4_switch_ID << " Running CLI commands from "
                        << commandsFile);

  int port = get_runtime_port ();
  bm_runtime::start_server (this, port);
  // start_and_return ();
  NS_LOG_DEBUG ("Switch ID: " << p4_switch_ID << " Waiting for the runtime server to start");
  std::this_thread::sleep_for (std::chrono::seconds (3));

  // Run the CLI commands to populate table entries
  std::string cmd = "run_bmv2_CLI --thrift_port " + std::to_string (port) + " " + commandsFile;
  std::system (cmd.c_str ());
}

int
P4CoreV1model::receive_ (uint32_t port_num, const char *buffer, int len)
{
  NS_LOG_FUNCTION (this << " Switch ID: " << p4_switch_ID << " Port: " << port_num
                        << " Len: " << len);
  return 0;
}

void
P4CoreV1model::start_and_return_ ()
{
  NS_LOG_FUNCTION ("Switch ID: " << p4_switch_ID << " start");
  CheckQueueingMetadata ();

  if (!egress_time_reference.IsZero ())
    {
      NS_LOG_DEBUG (
          "Switch ID: " << p4_switch_ID
                        << " Scheduling initial timer event using egress_time_reference = "
                        << egress_time_reference.GetNanoSeconds () << " ns");
      egress_timer_event =
          Simulator::Schedule (egress_time_reference, &P4CoreV1model::SetEgressTimerEvent, this);
    }
}

void
P4CoreV1model::SetEgressTimerEvent ()
{
  NS_LOG_FUNCTION ("p4_switch has been triggered by the egress timer event");
  static bool m_firstRun = false;
  bool checkflag = ProcessEgress (0);
  egress_timer_event =
      Simulator::Schedule (egress_time_reference, &P4CoreV1model::SetEgressTimerEvent, this);
  if (!m_firstRun && checkflag)
    {
      m_firstRun = true;
    }
  if (m_firstRun && !checkflag)
    {
      NS_LOG_INFO ("Egress timer event needs additional scheduling due to !checkflag.");
      Simulator::Schedule (Time (NanoSeconds (10)), &P4CoreV1model::ProcessEgress, this, 0);
    }
}

uint64_t
P4CoreV1model::GetTimeStamp ()
{
  return Simulator::Now ().GetNanoSeconds () - start_timestamp;
}

void
P4CoreV1model::swap_notify_ ()
{
  NS_LOG_FUNCTION ("p4_switch has been notified of a config swap");
  CheckQueueingMetadata ();
}

void
P4CoreV1model::CheckQueueingMetadata ()
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
          NS_LOG_WARN ("Switch ID: "
                       << p4_switch_ID
                       << " Your JSON input defines some but not all queueing metadata fields");
        }
    }
  else
    {
      NS_LOG_WARN (
          "Switch ID: " << p4_switch_ID
                        << " Your JSON input does not define any queueing metadata fields");
    }
  with_queueing_metadata = false;
}

void
P4CoreV1model::reset_target_state_ ()
{
  NS_LOG_DEBUG ("Resetting simple_switch target-specific state");
  get_component<bm::McSimplePreLAG> ()->reset_state ();
}

void
P4CoreV1model::CalculateScheduleTime ()
{
  egress_timer_event = EventId ();

  uint64_t bottleneck_ns = 1e9 / packet_rate_pps;
  egress_buffer.set_rate_for_all (packet_rate_pps);
  egress_time_reference = Time::FromDouble (bottleneck_ns, Time::NS);

  NS_LOG_DEBUG ("Switch ID: " << p4_switch_ID << " Egress time reference set to " << bottleneck_ns
                              << " ns (" << egress_time_reference.GetNanoSeconds () << " [ns])");
}

int
P4CoreV1model::ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
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

  // setting ns3 specific metadata in packet register
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
      phv->get_field ("intrinsic_metadata.ingress_global_timestamp").set (GetTimeStamp ());
    }

  input_buffer->push_front (InputBuffer::PacketType::NORMAL, std::move (bm_packet));
  ProcessIngress ();
  NS_LOG_DEBUG ("Packet received by P4CoreV1model, Port: " << inPort << ", Packet ID: " << packet_id
                                                           << ", Size: " << len << " bytes");
  return 0;
}

void
P4CoreV1model::Enqueue (port_t egress_port, std::unique_ptr<bm::Packet> &&packet)
{
  packet->set_egress_port (egress_port);

  bm::PHV *phv = packet->get_phv ();

  if (with_queueing_metadata)
    {
      phv->get_field ("queueing_metadata.enq_timestamp").set (GetTimeStamp ());
      phv->get_field ("queueing_metadata.enq_qdepth").set (egress_buffer.size (egress_port));
    }

  size_t priority = phv->has_field (SSWITCH_PRIORITY_QUEUEING_SRC)
                        ? phv->get_field (SSWITCH_PRIORITY_QUEUEING_SRC).get<size_t> ()
                        : 0u;
  if (priority >= nb_queues_per_port)
    {
      NS_LOG_ERROR ("Priority out of range, dropping packet");
      return;
    }

  egress_buffer.push_front (egress_port, nb_queues_per_port - 1 - priority, std::move (packet));

  NS_LOG_DEBUG ("Packet enqueued in P4QueueDisc, Port: " << egress_port
                                                         << ", Priority: " << priority);
}

void
P4CoreV1model::ProcessIngress ()
{
  NS_LOG_FUNCTION (this);

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
          GetMirroringSession (static_cast<int> (clone_mirror_session_id), &config);
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
          CopyFieldList (bm_packet, bm_packet_copy, PKT_INSTANCE_TYPE_INGRESS_CLONE, field_list_id);
          if (config.mgid_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to MGID {}" << config.mgid);
              MulticastPacket (bm_packet_copy.get (), config.mgid);
            }
          if (config.egress_port_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to egress port "
                            << config.egress_port << ", Packet ID: " << bm_packet->get_packet_id ()
                            << ", Size: " << bm_packet->get_data_size () << " bytes");
              Enqueue (config.egress_port, std::move (bm_packet_copy));
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
      CopyFieldList (bm_packet, bm_packet_copy, PKT_INSTANCE_TYPE_RESUBMIT, field_list_id);
      RegisterAccess::clear_all (bm_packet_copy.get ());
      bm_packet_copy->set_register (RegisterAccess::PACKET_LENGTH_REG_IDX, ingress_packet_size);
      phv_copy->get_field ("standard_metadata.packet_length").set (ingress_packet_size);

      input_buffer->push_front (InputBuffer::PacketType::RESUBMIT, std::move (bm_packet_copy));
      ProcessIngress ();
      return;
    }

  // MULTICAST
  if (mgid != 0)
    {
      NS_LOG_DEBUG ("Multicast requested for packet");
      auto &f_instance_type = phv->get_field ("standard_metadata.instance_type");
      f_instance_type.set (PKT_INSTANCE_TYPE_REPLICATION);
      MulticastPacket (bm_packet.get (), mgid);
      // when doing MulticastPacket, we discard the original packet
      return;
    }

  uint32_t egress_port = egress_spec;
  NS_LOG_DEBUG ("Egress port is " << egress_port);

  if (egress_port == SSWITCH_DROP_PORT)
    {
      // drop packet
      NS_LOG_DEBUG ("Dropping packet at the end of ingress");
      return;
    }
  auto &f_instance_type = phv->get_field ("standard_metadata.instance_type");
  f_instance_type.set (PKT_INSTANCE_TYPE_NORMAL);

  Enqueue (egress_port, std::move (bm_packet));
}

bool
P4CoreV1model::ProcessEgress (size_t worker_id)
{
  NS_LOG_FUNCTION ("Dequeue packet from QueueBuffer");
  std::unique_ptr<bm::Packet> bm_packet;
  size_t port;
  size_t priority;

  int queue_number = SSWITCH_VIRTUAL_QUEUE_NUM;

  for (int i = 0; i < queue_number; i++)
    {
      if (egress_buffer.size (i) > 0)
        {
          break;
        }
      if (i == queue_number - 1)
        {
          return false;
        }
    }

  egress_buffer.pop_back (worker_id, &port, &priority, &bm_packet);
  if (bm_packet == nullptr)
    return false;

  NS_LOG_FUNCTION ("Egress processing for the packet");
  bm::PHV *phv = bm_packet->get_phv ();
  bm::Pipeline *egress_mau = this->get_pipeline ("egress");
  bm::Deparser *deparser = this->get_deparser ("deparser");

  if (phv->has_field ("intrinsic_metadata.egress_global_timestamp"))
    {
      phv->get_field ("intrinsic_metadata.egress_global_timestamp").set (GetTimeStamp ());
    }

  if (with_queueing_metadata)
    {
      uint64_t enq_timestamp = phv->get_field ("queueing_metadata.enq_timestamp").get<uint64_t> ();
      phv->get_field ("queueing_metadata.deq_timedelta").set (GetTimeStamp () - enq_timestamp);

      size_t priority = phv->has_field (SSWITCH_PRIORITY_QUEUEING_SRC)
                            ? phv->get_field (SSWITCH_PRIORITY_QUEUEING_SRC).get<size_t> ()
                            : 0u;
      if (priority >= nb_queues_per_port)
        {
          NS_LOG_ERROR ("Priority out of range (nb_queues_per_port = " << nb_queues_per_port
                                                                       << "), dropping packet");
          return true;
        }

      phv->get_field ("queueing_metadata.deq_qdepth").set (egress_buffer.size (port));
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
          GetMirroringSession (static_cast<int> (clone_mirror_session_id), &config);
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
              MulticastPacket (packet_copy.get (), config.mgid);
            }
          if (config.egress_port_valid)
            {
              NS_LOG_DEBUG ("Cloning packet to egress port " << config.egress_port);
              // TODO This create a copy packet(new bm packet), but the UID mapping may need to
              // be updated in map.
              Enqueue (config.egress_port, std::move (packet_copy));
            }
        }
    }

  // TODO(antonin): should not be done like this in egress pipeline
  uint32_t egress_spec = f_egress_spec.get_uint ();
  if (egress_spec == SSWITCH_DROP_PORT)
    {
      // drop packet
      NS_LOG_DEBUG ("Dropping packet at the end of egress");
      return true;
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
      return true;
    }

  uint16_t protocol = RegisterAccess::get_ns_protocol (bm_packet.get ());
  int addr_index = RegisterAccess::get_ns_address (bm_packet.get ());

  Ptr<Packet> ns_packet = this->ConvertToNs3Packet (std::move (bm_packet));
  bridge_net_device->SendNs3Packet (ns_packet, port, protocol, destination_list[addr_index]);
  return true;
}

Ptr<Packet>
P4CoreV1model::ConvertToNs3Packet (std::unique_ptr<bm::Packet> &&bm_packet)
{
  // Create a new ns3::Packet using the data buffer
  char *bm_buf = bm_packet.get ()->data ();
  size_t len = bm_packet.get ()->get_data_size ();
  Ptr<Packet> ns_packet = Create<Packet> ((uint8_t *) (bm_buf), len);

  return ns_packet;
}

void
P4CoreV1model::CopyFieldList (const std::unique_ptr<bm::Packet> &packet,
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
P4CoreV1model::MulticastPacket (bm::Packet *packet, unsigned int mgid)
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
      Enqueue (egress_port, std::move (packet_copy));
    }
}

bool
P4CoreV1model::AddMirroringSession (int mirror_id, const MirroringSessionConfig &config)
{
  return mirroring_sessions->add_session (mirror_id, config);
}

bool
P4CoreV1model::DeleteMirroringSession (int mirror_id)
{
  return mirroring_sessions->delete_session (mirror_id);
}

bool
P4CoreV1model::GetMirroringSession (int mirror_id, MirroringSessionConfig *config) const
{
  return mirroring_sessions->get_session (mirror_id, config);
}

int
P4CoreV1model::SetEgressPriorityQueueDepth (size_t port, size_t priority, const size_t depth_pkts)
{
  egress_buffer.set_capacity (port, priority, depth_pkts);
  return 0;
}

int
P4CoreV1model::SetEgressQueueDepth (size_t port, const size_t depth_pkts)
{
  egress_buffer.set_capacity (port, depth_pkts);
  return 0;
}

int
P4CoreV1model::SetAllEgressQueueDepths (const size_t depth_pkts)
{
  egress_buffer.set_capacity_for_all (depth_pkts);
  return 0;
}

int
P4CoreV1model::SetEgressPriorityQueueRate (size_t port, size_t priority, const uint64_t rate_pps)
{
  egress_buffer.set_rate (port, priority, rate_pps);
  return 0;
}

int
P4CoreV1model::SetEgressQueueRate (size_t port, const uint64_t rate_pps)
{
  egress_buffer.set_rate (port, rate_pps);
  return 0;
}

int
P4CoreV1model::SetAllEgressQueueRates (const uint64_t rate_pps)
{
  egress_buffer.set_rate_for_all (rate_pps);
  return 0;
}

int
P4CoreV1model::GetAddressIndex (const Address &destination)
{
  auto it = address_map.find (destination);
  if (it != address_map.end ())
    {
      return it->second;
    }
  else
    {
      int new_index = destination_list.size ();
      destination_list.push_back (destination);
      address_map[destination] = new_index;
      return new_index;
    }
}

void
P4CoreV1model::PrintSwitchConfig ()
{
  std::cout << "\n========== Switch Configuration ==========\n";
  std::cout << "Thrift Port:             " << get_runtime_port () << "\n";
  std::cout << "Switch ID:               " << p4_switch_ID << "\n";
  std::cout << "Drop Port:               " << static_cast<int> (drop_port) << "\n";
  std::cout << "Queues Per Port:         " << nb_queues_per_port << "\n";
  std::cout << "Queueing Metadata:       " << (with_queueing_metadata ? "Enabled" : "Disabled")
            << "\n";
  std::cout << "Packet Processing Rate:  " << packet_rate_pps << " PPS\n";
  std::cout << "Start Timestamp:         " << start_timestamp << "\n";
}

} // namespace ns3
