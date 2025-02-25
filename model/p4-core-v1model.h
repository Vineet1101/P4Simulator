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

#ifndef P4_CORE_V1MODEL_H
#define P4_CORE_V1MODEL_H

#include "ns3/delay-jitter-estimation.h"
#include "ns3/p4-queue.h"
#include "ns3/p4-switch-net-device.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/switch.h>
#include <map>
#include <vector>

#define SSWITCH_PRIORITY_QUEUEING_SRC "intrinsic_metadata.priority"
#define SSWITCH_OUTPUT_BUFFER_SIZE 1024
#define SSWITCH_DROP_PORT 511
#define SSWITCH_VIRTUAL_QUEUE_NUM 8

namespace ns3
{

class P4SwitchNetDevice;
class InputBuffer;

/**
 * @brief A P4 Pipeline Implementation to be wrapped in P4 Device
 *
 * The P4CoreV1model uses the pipeline implementation provided by
 * `Behavioral Model` (https://github.com/p4lang/behavioral-model).
 * Internal processing functions and the `switch` class are used.
 * However, P4CoreV1model processes packets in a way adapted to ns-3.
 *
 * P4CoreV1model is initialized along with the P4 Device and exposes a public
 * function called `ReceivePacket()` to handle incoming packets.
 */
class P4CoreV1model : public bm::Switch
{
  public:
    // === Constructor & Destructor ===
    P4CoreV1model(P4SwitchNetDevice* netDevice,
                  bool enableSwap,
                  bool enableTracing,
                  uint64_t packet_rate,
                  size_t input_buffer_size_low,
                  size_t input_buffer_size_high,
                  size_t queue_buffer_size,
                  port_t drop_port = SSWITCH_DROP_PORT,
                  size_t queues_per_port = SSWITCH_VIRTUAL_QUEUE_NUM);
    ~P4CoreV1model();

    // === Public Methods ===
    void InitSwitchWithP4(std::string jsonPath, std::string flowTablePath);
    void RunCli(const std::string& commandsFile);
    void EnableTracing();
    int InitFromCommandLineOptions(int argc, char* argv[]);
    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);
    void SetEgressTimerEvent();
    Ptr<Packet> ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bmPacket);
    void CalculateScheduleTime();
    uint64_t GetTimeStamp(); // Timers
    void PrintSwitchConfig();

    void CalculatePacketsPerSecond();

    // === override ===
    int receive_(uint32_t port_num, const char* buffer, int len) override;
    void start_and_return_() override;
    void swap_notify_() override;
    void reset_target_state_() override;

    // Queue Configuration
    int SetEgressPriorityQueueDepth(size_t port, size_t priority, size_t depthPkts);
    int SetEgressQueueDepth(size_t port, size_t depthPkts);
    int SetAllEgressQueueDepths(size_t depthPkts);
    int SetEgressPriorityQueueRate(size_t port, size_t priority, uint64_t ratePps);
    int SetEgressQueueRate(size_t port, uint64_t ratePps);
    int SetAllEgressQueueRates(uint64_t ratePps);

    // Disabling copy and move operations
    P4CoreV1model(const P4CoreV1model&) = delete;
    P4CoreV1model& operator=(const P4CoreV1model&) = delete;
    P4CoreV1model(P4CoreV1model&&) = delete;
    P4CoreV1model&& operator=(P4CoreV1model&&) = delete;

  protected:
    // Mirroring session configuration
    struct MirroringSessionConfig
    {
        port_t egress_port;
        bool egress_port_valid;
        unsigned int mgid;
        bool mgid_valid;
    };

    bool AddMirroringSession(int mirrorId, const MirroringSessionConfig& config);
    bool DeleteMirroringSession(int mirrorId);
    bool GetMirroringSession(int mirrorId, MirroringSessionConfig* config) const;

    struct EgressThreadMapper
    {
        explicit EgressThreadMapper(size_t nb_threads)
            : nb_threads(nb_threads)
        {
        }

        size_t operator()(size_t egress_port) const
        {
            return egress_port % nb_threads;
        }

        size_t nb_threads;
    };

    enum PktInstanceType
    {
        PKT_INSTANCE_TYPE_NORMAL,
        PKT_INSTANCE_TYPE_INGRESS_CLONE,
        PKT_INSTANCE_TYPE_EGRESS_CLONE,
        PKT_INSTANCE_TYPE_COALESCED,
        PKT_INSTANCE_TYPE_RECIRC,
        PKT_INSTANCE_TYPE_REPLICATION,
        PKT_INSTANCE_TYPE_RESUBMIT,
    };

    // Internal Methods
    void CheckQueueingMetadata();
    void ProcessIngress();
    void Enqueue(port_t egress_port, std::unique_ptr<bm::Packet>&& packet);
    bool ProcessEgress(size_t workerId);
    void TransmitEvent();
    void MulticastPacket(bm::Packet* packet, unsigned int mgid);

    // Utility Methods
    int GetAddressIndex(const Address& destination);
    void CopyFieldList(const std::unique_ptr<bm::Packet>& packet,
                       const std::unique_ptr<bm::Packet>& packetCopy,
                       PktInstanceType copyType,
                       int fieldListId);

  private:
    bool m_enableTracing;
    uint64_t m_packetId; // Packet ID

    int m_p4SwitchId; //!< ID of the switch
    int m_thriftPort;
    uint64_t m_bitsPerTimeInterval;    // bps
    double m_totalPacketRate;          // pps
    uint64_t m_packetsPerTimeInterval; // pps
    uint64_t m_totalPakcketsNum;
    Time m_timeInterval;       // s
    double m_virtualQueueRate; // pps

    P4SwitchNetDevice* m_switchNetDevice;

    class MirroringSessions;

    port_t m_dropPort; //!< Port to drop packets
    size_t m_nbQueuesPerPort;
    bool m_enableQueueingMetadata{true};
    static constexpr size_t m_nbEgressThreads = 1u; // 4u default in bmv2
    uint64_t m_switchRate;     //!< Switch processing capability (unit: PPS (Packets
                               //!< Per Second))
    EventId m_egressTimeEvent; //!< The timer event ID for dequeue
    Time m_egressTimeRef;      //!< Desired time between timer event triggers
    uint64_t m_startTimestamp; //!< Start time of the switch

    // Buffers and Transmit Function
    std::unique_ptr<InputBuffer> input_buffer;
    NSQueueingLogicPriRL<std::unique_ptr<bm::Packet>, EgressThreadMapper> egress_buffer;
    bm::Queue<std::unique_ptr<bm::Packet>> output_buffer;

    // Miscellaneous
    bm::TargetParserBasic* m_argParser;     //!< Structure of parsers
    std::vector<Address> m_destinationList; //!< List of addresses (O(log n) search)
    std::map<Address, int> m_addressMap;    //!< Map for fast lookup
    std::shared_ptr<bm::McSimplePreLAG> m_pre;
    std::unique_ptr<MirroringSessions> m_mirroringSessions;

    bool m_firstPacket;
};

} // namespace ns3

#endif // !P4_CORE_V1MODEL_H