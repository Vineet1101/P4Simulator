/* Copyright 2013-present Barefoot Networks, Inc.
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
 *
 * Authors: Antonin Bas<antonin@barefootnetworks.com>
 * Modified: Mingyu Ma<mingyu.ma@tu-dresden.de>
 */

#ifndef P4_CORE_PSA_H
#define P4_CORE_PSA_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-net-device.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <map>
#include <vector>
// #include <bm/bm_sim/counters.h>
#include <bm/bm_sim/simple_pre_lag.h>

#define SSWITCH_OUTPUT_BUFFER_SIZE 1024
#define SSWITCH_DROP_PORT 511 // @TODO this port should be same with Netdevice
#define SSWITCH_VIRTUAL_QUEUE_NUM 8

namespace ns3
{

class P4SwitchNetDevice;
class InputBuffer;

/**
 * @brief A P4 Pipeline Implementation to be wrapped in P4 Device
 *
 * The P4CorePsa uses the pipeline implementation provided by
 * `Behavioral Model` (https://github.com/p4lang/behavioral-model).
 * Internal processing functions and the `switch` class are used.
 * However, P4CorePsa processes packets in a way adapted to ns-3.
 *
 * P4CorePsa is initialized along with the P4 Device and exposes a public
 * function called `ReceivePacket()` to handle incoming packets.
 */
class P4CorePsa : public bm::Switch
{
  public:
    // === Static Methods ===
    static TypeId GetTypeId(void);

    // === Constructor & Destructor ===
    P4CorePsa(P4SwitchNetDevice* netDevice,
              bool enable_swap,
              bool enable_tracing,
              uint64_t packet_rate,
              size_t input_buffer_size,
              size_t queue_buffer_size,
              port_t drop_port = SSWITCH_DROP_PORT,
              size_t nb_queues_per_port = SSWITCH_VIRTUAL_QUEUE_NUM); // by default, swapping is off
    ~P4CorePsa();

    // === Public Methods ===
    void RunCli(const std::string& commandsFile);
    void InitSwitchWithP4(std::string jsonPath, std::string flowTablePath);
    int InitFromCommandLineOptions(int argc, char* argv[]);
    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);
    void SetEgressTimerEvent();
    Ptr<Packet> ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bmPacket);
    uint64_t GetTimeStamp(); // Timers
    void CalculateScheduleTime();

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
    P4CorePsa(const P4CorePsa&) = delete;
    P4CorePsa& operator=(const P4CorePsa&) = delete;
    P4CorePsa(P4CorePsa&&) = delete;
    P4CorePsa&& operator=(P4CorePsa&&) = delete;

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
        PACKET_PATH_NORMAL,
        PACKET_PATH_NORMAL_UNICAST,
        PACKET_PATH_NORMAL_MULTICAST,
        PACKET_PATH_CLONE_I2E,
        PACKET_PATH_CLONE_E2E,
        PACKET_PATH_RESUBMIT,
        PACKET_PATH_RECIRCULATE,
    };

    // Internal Methods
    void CheckQueueingMetadata();
    void ProcessIngress();
    void Enqueue(port_t egress_port, std::unique_ptr<bm::Packet>&& packet);
    bool ProcessEgress(size_t workerId);
    void TransmitEvent();
    void MultiCastPacket(bm::Packet* packet,
                         unsigned int mgid,
                         PktInstanceType path,
                         unsigned int class_of_service);

    // Utility Methods
    int GetAddressIndex(const Address& destination);

  private:
    static constexpr port_t PSA_PORT_RECIRCULATE = 0xfffffffa;
    static constexpr size_t nb_egress_threads = 1u; // 4u default
    uint64_t m_packetId;                            // Packet ID

    int m_p4SwitchId; //!< ID of the switch
    int m_thriftPort;

    P4SwitchNetDevice* m_switchNetDevice;
    bool m_enableTracing;
    port_t m_dropPort; //!< Port to drop packets
    size_t m_nbQueuesPerPort;
    bool m_enableQueueingMetadata{true};

    class MirroringSessions;

    EventId m_egressTimeEvent; //!< The timer event ID [Egress]
    Time m_egressTimeRef;      //!< Desired time between timer event triggers
    uint64_t m_switchRate;     //!< Switch processing capability (unit: PPS (Packets
                               //!< Per Second))
    uint64_t m_startTimestamp; //!< Start time of the switch (clock::time_point start;)

    // Buffers and Transmit Function
    // std::unique_ptr<InputBuffer> input_buffer;
    bm::Queue<std::unique_ptr<bm::Packet>> input_buffer;
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

#endif // !P4_CORE_PSA_H