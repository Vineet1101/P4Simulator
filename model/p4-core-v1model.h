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
    /**
     * @brief Receive a packet from the switch net device
     * @details This function is called by the switch net device to pass a packet
     * to the switch core for processing.
     * @param packetIn The packet to be processed
     * @param inPort The ingress port of the packet
     * @param protocol The protocol of the packet
     * @param destination The destination address of the packet
     * @return int 0 if successful
     */
    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination) override;

    /**
     * @brief Notified of a config swap
     */
    void swap_notify_() override;

    /**
     * @brief Start the switch and return
     * Set the scheduler time with the switch rate
     */
    void start_and_return_() override;

    /**
     * @brief Reset the target state
     */
    void reset_target_state_() override;

    /**
     * @brief Handle the ingress pipeline
     */
    void HandleIngressPipeline() override;

    /**
     * @brief Enqueue a packet to the queue buffer between ingress and egress
     * @param egress_port The egress port of the packet
     * @param packet The packet to be enqueued
     * @return void
     */
    void Enqueue(uint32_t egress_port, std::unique_ptr<bm::Packet>&& packet) override;

    /**
     * @brief Handle the egress pipeline
     * @param workerId The worker ID of the egress pipeline
     * @return bool True if the egress pipeline is handled successfully
     * @return bool False if the egress pipeline needs additional scheduling
     */
    bool HandleEgressPipeline(size_t workerId) override;

    /**
     * @brief Calculate the schedule time for the egress pipeline
     */
    void CalculateScheduleTime();

    /**
     * @brief Set the egress timer event
     * @details This function is called by the egress timer event to trigger the
     * egress pipeline
     */
    void CalculatePacketsPerSecond();

    /**
     * @brief Set the egress timer event
     * @details This function is called by the egress timer event to trigger the
     * dequeue, then run the egress pipeline
     */
    void SetEgressTimerEvent();

    /**
     * @brief Multicast a packet to a multicast group ID
     * @param packet The packet to be multicast
     * @param mgid The multicast group ID
     */
    void MulticastPacket(bm::Packet* packet, unsigned int mgid);

    /**
     * @brief Used for ingress cloning, resubmit
     */
    void CopyFieldList(const std::unique_ptr<bm::Packet>& packet,
                       const std::unique_ptr<bm::Packet>& packetCopy,
                       PktInstanceTypeV1model copyType,
                       int fieldListId);

    /**
     * @brief Set the depth of a priority queue
     * @param port The egress port
     * @param priority The priority of the queue
     * @param depthPkts The depth of the queue in packets
     * @return int 0 if successful
     */
    int SetEgressPriorityQueueDepth(size_t port, size_t priority, size_t depthPkts);

    /**
     * @brief Set the depth of a queue
     * @param port The egress port
     * @param depthPkts The depth of the queue in packets
     * @return int 0 if successful
     */
    int SetEgressQueueDepth(size_t port, size_t depthPkts);

    /**
     * @brief Set the depth of all queues
     * @param depthPkts The depth of the queue in packets
     * @return int 0 if successful
     */
    int SetAllEgressQueueDepths(size_t depthPkts);

    /**
     * @brief Set the rate of a priority queue
     * @param port The egress port
     * @param priority The priority of the queue
     * @param ratePps The rate of the queue in packets per second
     * @return int 0 if successful
     */
    int SetEgressPriorityQueueRate(size_t port, size_t priority, uint64_t ratePps);

    /**
     * @brief Set the rate of a virtual queue
     * @param port The egress port
     * @param ratePps The rate of the queue in packets per second
     * @return int 0 if successful
     */
    int SetEgressQueueRate(size_t port, uint64_t ratePps);

    /**
     * @brief Set the rate of all virtual queues
     * @param ratePps The rate of the queue in packets per second
     * @return int 0 if successful
     */
    int SetAllEgressQueueRates(uint64_t ratePps);

  protected:
    /**
     * @brief The egress thread mapper for dequeue process of queue buffer
     * bmv2 using 4u default, in ns-3 only single thread for processing
     */
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

    // enable tracing
    uint64_t m_inputBps;  // bps
    uint64_t m_inputBp;   // bp
    uint64_t m_inputPps;  // pps
    uint64_t m_inputPp;   // pp
    uint64_t m_egressBps; // bps
    uint64_t m_egressBp;  // bp
    uint64_t m_egressPps; // pps
    uint64_t m_egressPp;  // pp

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
