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

#ifndef P4_SWITCH_CORE_H
#define P4_SWITCH_CORE_H

#include "bridge-p4-net-device.h"
#include "ns3/p4-queue-item.h"
#include "ns3/p4-queue.h"

// #include "ns3/register_access.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>

namespace ns3 {

class BridgeP4NetDevice;

// struct PacketInfo
// {
//   int in_port;
//   uint16_t protocol;
//   Address destination;
//   u_int64_t packet_id;
// };

struct MirroringSessionConfig
{
  uint32_t egress_port;
  bool egress_port_valid;
  unsigned int mgid;
  bool mgid_valid;
};

class MirroringSessions
{
public:
  bool AddSession (int mirror_id, const MirroringSessionConfig &config);

  bool DeleteSession (int mirror_id);

  bool GetSession (int mirror_id, MirroringSessionConfig *config) const;

private:
  std::unordered_map<int, MirroringSessionConfig> sessions_map;
};

/**
 * \ingroup p4-pipeline
 *
 * Base class for a P4 programmable pipeline.
 */
class P4Switch : public bm::Switch
{
public:
  P4Switch (BridgeP4NetDevice *netDevice);

  static TypeId GetTypeId (void);

  ~P4Switch ();

  void run_cli (std::string commandsFile);
  
  int receive_ (uint32_t port_num, const char *buffer, int len) override;

  void start_and_return_ () override;

  // void reset_target_state_() override;

  void swap_notify_ () override;

  bool mirroring_add_session (int mirror_id, const MirroringSessionConfig &config);

  bool mirroring_delete_session (int mirror_id);

  bool mirroring_get_session (int mirror_id, MirroringSessionConfig *config) const;

  int ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                     const Address &destination);

  // !!! Deprecated function, see p4-switch-interface.cc for the new init function
  // int init(int argc, char* argv[]);

  /**
     * \brief configure switch with json file
     */
  int InitFromCommandLineOptionsLocal (int argc, char *argv[]);

  void packets_process_pipeline (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                                 const Address &destination);

  void push_input_buffer (Ptr<Packet> packetIn);

  void push_input_buffer (std::unique_ptr<bm::Packet> &&bm_packet, PacketType packet_type);

  void push_transmit_buffer (std::unique_ptr<bm::Packet> &&bm_packet);

  void parser_ingress_processing ();

  void enqueue (uint32_t egress_port, std::unique_ptr<bm::Packet> &&bm_packet);

  void egress_deparser_processing ();

  void multicast (bm::Packet *packet, unsigned int mgid);

  void check_queueing_metadata ();

  void
  SetSkipTracing (bool skipTracing)
  {
    skip_tracing = skipTracing;
  }

  bool AddVritualQueue (uint32_t port_num);

  bool RemoveVirtualQueue (uint32_t port_num);

  std::unique_ptr<bm::Packet> get_bm_packet (Ptr<P4QueueItem> item);
  std::unique_ptr<bm::Packet> get_bm_packet_from_ingress (Ptr<Packet> ns_packet, uint16_t in_port);
  // Ptr<Packet> get_ns3_packet (std::unique_ptr<bm::Packet> bm_packet);

  Ptr<P4QueueItem> get_ns3_packet_queue_item (std::unique_ptr<bm::Packet> bm_packet,
                                              PacketType packet_type);
  P4Switch (const P4Switch &) = delete;
  P4Switch &operator= (const P4Switch &) = delete;
  P4Switch (P4Switch &&) = delete;
  P4Switch &&operator= (P4Switch &&) = delete;

  static constexpr uint32_t default_drop_port = 511;

protected:
  static bm::packet_id_t packet_id;
  static int thrift_port;

private:
  // class MirroringSessions;

  enum PktInstanceType {
    PKT_INSTANCE_TYPE_NORMAL,
    PKT_INSTANCE_TYPE_INGRESS_CLONE,
    PKT_INSTANCE_TYPE_EGRESS_CLONE,
    PKT_INSTANCE_TYPE_COALESCED,
    PKT_INSTANCE_TYPE_RECIRC,
    PKT_INSTANCE_TYPE_REPLICATION,
    PKT_INSTANCE_TYPE_RESUBMIT,
  };

  void copy_field_list_and_set_type (const std::unique_ptr<bm::Packet> &packet,
                                     const std::unique_ptr<bm::Packet> &packet_copy,
                                     PktInstanceType copy_type, int field_list_id);

  bm::TargetParserBasic *m_argParser; //!< Structure of parsers

  int p4_switch_ID; //!< ID of the switch

  // time event for processing
  EventId m_ingressTimerEvent; //!< The timer event ID [Ingress]
  Time m_ingressTimeReference; //!< Desired time between timer event triggers
  EventId m_egressTimerEvent; //!< The timer event ID [Egress]
  Time m_egressTimeReference; //!< Desired time between timer event triggers

  bool skip_tracing = true; // whether to skip tracing
  bool with_queueing_metadata{true}; // whether to include queueing metadata

  size_t nb_queues_per_port{8}; // 3 bit for the queue number, max value is 8

  std::unique_ptr<MirroringSessions> mirroring_sessions;

  std::vector<Address> destination_list; //!< list for address, using by index

  Ptr<TwoTierP4Queue> input_buffer;
  Ptr<P4Queuebuffer> queue_buffer;
  // Ptr<FifoQueueDisc> transmit_buffer;

  BridgeP4NetDevice *m_pNetDevice;
};

} // namespace ns3

#endif // !P4_SWITCH_CORE_H