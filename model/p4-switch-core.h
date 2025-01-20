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
#include "ns3/p4-queue.h"
#include "ns3/delay-jitter-estimation.h"

#include <map>
#include <vector>

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/simple_pre_lag.h>

#define SSWITCH_PRIORITY_QUEUEING_SRC "intrinsic_metadata.priority"

namespace ns3 {

class BridgeP4NetDevice;
class InputBuffer;

/**
* @brief A P4 Pipeline Implementation to be wrapped in P4 Device
*
* The P4Switch is using pipeline implementation provided by
* `Behavioral Model` (https://github.com/p4lang/behavioral-model).
* In particular, some internal processing functions and the `switch`
* class are used. However, the way P4Switch processes packets is
* adapted to the requirements of ns-3.
*
* P4Switch is initialized along with P4 Device, and expose a public
* function called \p receivePacket() to the P4 Device. Whenever P4
* Device has a packet needing handling, it call receivePacket and
* wait for this function to return. \p receivePacket() puts the packet
* through P4 pipeline and returns when the packet is ready to be
*
*/
class P4Switch : public bm::Switch
{
public:
  static constexpr port_t default_drop_port = 511;
  static constexpr size_t default_nb_queues_per_port = 8;

  using TransmitFn = std::function<void (port_t, uint64_t, const char *, int)>;

  static TypeId GetTypeId (void);
  P4Switch (BridgeP4NetDevice *netDevice, bool enable_swap = false,
            port_t drop_port = default_drop_port,
            size_t nb_queues_per_port = default_nb_queues_per_port);
  ~P4Switch ();

  struct MirroringSessionConfig
  {
    port_t egress_port;
    bool egress_port_valid;
    unsigned int mgid;
    bool mgid_valid;
  };

  struct EgressThreadMapper
  {
    explicit EgressThreadMapper (size_t nb_threads) : nb_threads (nb_threads)
    {
    }

    size_t
    operator() (size_t egress_port) const
    {
      return egress_port % nb_threads;
    }

    size_t nb_threads;
  };

  // void Init ();
  void run_cli (std::string commandsFile);

  int receive_ (uint32_t port_num, const char *buffer, int len) override;

  void start_and_return_ () override;

  void swap_notify_ () override;

  void reset_target_state_ () override;

  bool mirroring_add_session (int mirror_id, const MirroringSessionConfig &config);

  bool mirroring_delete_session (int mirror_id);

  bool mirroring_get_session (int mirror_id, MirroringSessionConfig *config) const;

  int ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                     const Address &destination);

  /**
     * \brief configure switch with json file
     */
  int InitFromCommandLineOptionsLocal (int argc, char *argv[]);

  void packets_process_pipeline (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                                 const Address &destination);

  void push_transmit_buffer (std::unique_ptr<bm::Packet> &&bm_packet);

  void parser_ingress_processing ();

  void enqueue (port_t egress_port, std::unique_ptr<bm::Packet> &&packet);

  void egress_deparser_processing (size_t worker_id);

  void multicast (bm::Packet *packet, unsigned int mgid);

  void check_queueing_metadata ();
  int set_egress_priority_queue_depth (size_t port, size_t priority, const size_t depth_pkts);
  int set_egress_queue_depth (size_t port, const size_t depth_pkts);
  int set_all_egress_queue_depths (const size_t depth_pkts);

  int set_egress_priority_queue_rate (size_t port, size_t priority, const uint64_t rate_pps);
  int set_egress_queue_rate (size_t port, const uint64_t rate_pps);
  int set_all_egress_queue_rates (const uint64_t rate_pps);

  void RunIngressTimerEvent ();
  void RunEgressTimerEvent ();

  Ptr<Packet> get_ns3_packet (std::unique_ptr<bm::Packet> &&bm_packet);

  int GetAddressIndex (const Address &destination);
  // const Address &GetAddressFromIndex (int index) const;

  P4Switch (const P4Switch &) = delete;
  P4Switch &operator= (const P4Switch &) = delete;
  P4Switch (P4Switch &&) = delete;
  P4Switch &&operator= (P4Switch &&) = delete;

protected:
  static int thrift_port;

private:
  class MirroringSessions;

  int p4_switch_ID; //!< ID of the switch

  size_t worker_id; //!< worker_id = threads_id, here only one

  // time event for processing
  // EventId m_ingressTimerEvent; //!< The timer event ID [Ingress]
  // Time m_ingressTimeReference; //!< Desired time between timer event triggers
  EventId m_egressTimerEvent; //!< The timer event ID [Egress]
  Time m_egressTimeReference; //!< Desired time between timer event triggers
  // EventId m_transmitTimerEvent; //!< The timer event ID [Transfer]
  // Time m_transmitTimeReference; //!< Desired time between timer event triggers

  static constexpr size_t nb_egress_threads =
      1u; // 4u default in bmv2, but in ns-3 remove the multi-thread
  static uint64_t packet_id;
  BridgeP4NetDevice *m_pNetDevice;

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

  std::vector<Address> destination_list; //!< List of addresses (O(log n) search)
  std::map<Address, int> address_map; //!< Map for fast lookup

  bool with_queueing_metadata{true};

  // INIT P4 SWITCH
  port_t drop_port; //!< Port to drop packets
  std::unique_ptr<InputBuffer> input_buffer;
  size_t nb_queues_per_port;
  NSQueueingLogicPriRL<std::unique_ptr<bm::Packet>, EgressThreadMapper> egress_buffers;
  bm::Queue<std::unique_ptr<bm::Packet>> output_buffer;
  TransmitFn my_transmit_fn;
  std::shared_ptr<bm::McSimplePreLAG> m_pre;
  std::chrono::high_resolution_clock::time_point start;
  std::unique_ptr<MirroringSessions> mirroring_sessions;
};

} // namespace ns3

#endif // !P4_SWITCH_CORE_H