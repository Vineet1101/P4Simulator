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

#include "ns3/bridge-p4-net-device.h"
#include "ns3/p4-queue.h"
#include "ns3/delay-jitter-estimation.h"

#include <map>
#include <vector>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/simple_pre_lag.h>

#define SSWITCH_PRIORITY_QUEUEING_SRC "intrinsic_metadata.priority"
#define SSWITCH_OUTPUT_BUFFER_SIZE 1024
#define SSWITCH_DROP_PORT 511
#define SSWITCH_VIRTUAL_QUEUE_NUM 8
namespace ns3 {

class BridgeP4NetDevice;
class InputBuffer;

/**
 * @brief A P4 Pipeline Implementation to be wrapped in P4 Device
 *
 * The P4Switch uses the pipeline implementation provided by
 * `Behavioral Model` (https://github.com/p4lang/behavioral-model).
 * Internal processing functions and the `switch` class are used.
 * However, P4Switch processes packets in a way adapted to ns-3.
 *
 * P4Switch is initialized along with the P4 Device and exposes a public
 * function called `ReceivePacket()` to handle incoming packets.
 */
class P4Switch : public bm::Switch
{
public:
  // === Constructor & Destructor ===
  P4Switch (BridgeP4NetDevice *netDevice, bool enableSwap, uint64_t packet_rate,
            size_t input_buffer_size_low, size_t input_buffer_size_high, size_t queue_buffer_size,
            port_t dropPort = SSWITCH_DROP_PORT, size_t queuesPerPort = SSWITCH_VIRTUAL_QUEUE_NUM);
  ~P4Switch ();

  // === Public Methods ===
  void InitSwitchWithP4 (std::string jsonPath, std::string flowTablePath);
  void RunCli (const std::string &commandsFile);
  int InitFromCommandLineOptions (int argc, char *argv[]);
  int ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                     const Address &destination);
  void SetEgressTimerEvent ();
  Ptr<Packet> ConvertToNs3Packet (std::unique_ptr<bm::Packet> &&bmPacket);

  // === override ===
  int receive_ (uint32_t port_num, const char *buffer, int len) override;
  void start_and_return_ () override;
  void swap_notify_ () override;
  void reset_target_state_ () override;

  // Queue Configuration
  int SetEgressPriorityQueueDepth (size_t port, size_t priority, size_t depthPkts);
  int SetEgressQueueDepth (size_t port, size_t depthPkts);
  int SetAllEgressQueueDepths (size_t depthPkts);
  int SetEgressPriorityQueueRate (size_t port, size_t priority, uint64_t ratePps);
  int SetEgressQueueRate (size_t port, uint64_t ratePps);
  int SetAllEgressQueueRates (uint64_t ratePps);

  void PrintSwitchConfig ();

  // Disabling copy and move operations
  P4Switch (const P4Switch &) = delete;
  P4Switch &operator= (const P4Switch &) = delete;
  P4Switch (P4Switch &&) = delete;
  P4Switch &&operator= (P4Switch &&) = delete;

protected:
  // Mirroring session configuration
  struct MirroringSessionConfig
  {
    port_t egress_port;
    bool egress_port_valid;
    unsigned int mgid;
    bool mgid_valid;
  };
  bool AddMirroringSession (int mirrorId, const MirroringSessionConfig &config);
  bool DeleteMirroringSession (int mirrorId);
  bool GetMirroringSession (int mirrorId, MirroringSessionConfig *config) const;

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

  enum PktInstanceType {
    PKT_INSTANCE_TYPE_NORMAL,
    PKT_INSTANCE_TYPE_INGRESS_CLONE,
    PKT_INSTANCE_TYPE_EGRESS_CLONE,
    PKT_INSTANCE_TYPE_COALESCED,
    PKT_INSTANCE_TYPE_RECIRC,
    PKT_INSTANCE_TYPE_REPLICATION,
    PKT_INSTANCE_TYPE_RESUBMIT,
  };

  // Internal Methods
  void CheckQueueingMetadata ();
  void ProcessIngress ();
  void Enqueue (port_t egress_port, std::unique_ptr<bm::Packet> &&packet);
  bool ProcessEgress (size_t workerId);
  void TransmitEvent ();
  void MulticastPacket (bm::Packet *packet, unsigned int mgid);

  // Utility Methods
  int GetAddressIndex (const Address &destination);
  void CopyFieldList (const std::unique_ptr<bm::Packet> &packet,
                      const std::unique_ptr<bm::Packet> &packetCopy, PktInstanceType copyType,
                      int fieldListId);

private:
  // static int thrift_port;
  int p4_switch_ID; //!< ID of the switch
  BridgeP4NetDevice *bridge_net_device;
  port_t drop_port; //!< Port to drop packets
  size_t nb_queues_per_port;

  bool with_queueing_metadata{true};
  static constexpr size_t nb_egress_threads = 1u; // 4u default in bmv2
  static uint64_t packet_id;

  class MirroringSessions;

  // Timers
  uint64_t GetTimeStamp ();

  uint64_t packet_rate_pps; //!< Switch processing capability (unit: PPS (Packets Per Second))
  EventId egress_timer_event; //!< The timer event ID for dequeue
  Time egress_time_reference; //!< Desired time between timer event triggers
  uint64_t start_timestamp; //!< Start time of the switch

  // Buffers and Transmit Function
  std::unique_ptr<InputBuffer> input_buffer;
  NSQueueingLogicPriRL<std::unique_ptr<bm::Packet>, EgressThreadMapper> egress_buffer;
  bm::Queue<std::unique_ptr<bm::Packet>> output_buffer;

  // Miscellaneous
  bm::TargetParserBasic *m_argParser; //!< Structure of parsers
  std::vector<Address> destination_list; //!< List of addresses (O(log n) search)
  std::map<Address, int> address_map; //!< Map for fast lookup
  std::shared_ptr<bm::McSimplePreLAG> m_pre;
  std::unique_ptr<MirroringSessions> mirroring_sessions;
};

} // namespace ns3

#endif // !P4_SWITCH_CORE_H