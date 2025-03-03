/* Copyright 2013-present Barefoot Networks, Inc.
 * Copyright 2021 VMware, Inc.
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
 * Authors: Antonin Bas <antonin@barefootnetworks.com>
 * Modified: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#ifndef P4_CORE_V1MODEL_H
#define P4_CORE_V1MODEL_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-core.h"

#define SSWITCH_VIRTUAL_QUEUE_NUM_V1MODEL 8

namespace ns3
{

class P4CoreV1model : public P4SwitchCore
{
  public:
    // === Constructor & Destructor ===
    P4CoreV1model(P4SwitchNetDevice* net_device,
                  bool enable_swap,
                  bool enableTracing,
                  uint64_t packet_rate,
                  size_t input_buffer_size_low,
                  size_t input_buffer_size_high,
                  size_t queue_buffer_size,
                  size_t nb_queues_per_port = SSWITCH_VIRTUAL_QUEUE_NUM_V1MODEL);
    ~P4CoreV1model();

    /**
     * @brief Different types of packet instances
     * @details The packet instance type is used to determine the processing
     * pipeline of the packet. PKT_INSTANCE_TYPE_NORMAL usually will have
     * lower priority than other types.
     */
    enum PktInstanceTypeV1model
    {
        PKT_INSTANCE_TYPE_NORMAL,
        PKT_INSTANCE_TYPE_INGRESS_CLONE,
        PKT_INSTANCE_TYPE_EGRESS_CLONE,
        PKT_INSTANCE_TYPE_COALESCED,
        PKT_INSTANCE_TYPE_RECIRC,
        PKT_INSTANCE_TYPE_REPLICATION,
        PKT_INSTANCE_TYPE_RESUBMIT,
    };

    // === Packet Processing ===
    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination) override;

    void swap_notify_() override;
    void start_and_return_() override;
    void reset_target_state_() override;

    void HandleIngressPipeline() override;
    void Enqueue(uint32_t egress_port, std::unique_ptr<bm::Packet>&& packet) override;
    bool HandleEgressPipeline(size_t workerId) override;

    void CalculateScheduleTime();
    void CalculatePacketsPerSecond();

    void SetEgressTimerEvent();
    void MulticastPacket(bm::Packet* packet, unsigned int mgid);

    void CopyFieldList(const std::unique_ptr<bm::Packet>& packet,
                       const std::unique_ptr<bm::Packet>& packetCopy,
                       PktInstanceTypeV1model copyType,
                       int fieldListId);

    // Queue Configuration
    int SetEgressPriorityQueueDepth(size_t port, size_t priority, size_t depthPkts);
    int SetEgressQueueDepth(size_t port, size_t depthPkts);
    int SetAllEgressQueueDepths(size_t depthPkts);
    int SetEgressPriorityQueueRate(size_t port, size_t priority, uint64_t ratePps);
    int SetEgressQueueRate(size_t port, uint64_t ratePps);
    int SetAllEgressQueueRates(uint64_t ratePps);

  protected:
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

  private:
    uint64_t m_packetId;
    uint64_t m_switchRate;

    uint64_t m_bitsPerTimeInterval;    // bps
    double m_totalPacketRate;          // pps
    uint64_t m_packetsPerTimeInterval; // pps
    uint64_t m_totalPakcketsNum;
    Time m_timeInterval;       // s
    double m_virtualQueueRate; // pps

    size_t m_nbQueuesPerPort;
    EventId m_egressTimeEvent; //!< The timer event ID for dequeue
    Time m_egressTimeRef;      //!< Desired time between timer event triggers
    uint64_t m_startTimestamp; //!< Start time of the switch

    static constexpr size_t m_nbEgressThreads = 1u; // 4u default in bmv2

    std::unique_ptr<InputBuffer> input_buffer;
    NSQueueingLogicPriRL<std::unique_ptr<bm::Packet>, EgressThreadMapper> egress_buffer;
    bm::Queue<std::unique_ptr<bm::Packet>> output_buffer;

    bool m_firstPacket;
};

} // namespace ns3

#endif // P4_CORE_V1MODEL_H
