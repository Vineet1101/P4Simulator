#ifndef P4_CORE_V1MODEL_DEV_H
#define P4_CORE_V1MODEL_DEV_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-core.h"

#include <map>
#include <vector>

#define SSWITCH_VIRTUAL_QUEUE_NUM_V1MODEL_DEV 8

namespace ns3
{

class P4CoreV1modelDev : public P4SwitchCore
{
  public:
    // === Constructor & Destructor ===
    P4CoreV1modelDev(P4SwitchNetDevice* net_device,
                     bool enable_swap,
                     bool enableTracing,
                     uint64_t packet_rate,
                     size_t input_buffer_size_low,
                     size_t input_buffer_size_high,
                     size_t queue_buffer_size,
                     size_t nb_queues_per_port = SSWITCH_VIRTUAL_QUEUE_NUM_V1MODEL_DEV);
    ~P4CoreV1modelDev();

    // === Packet Processing ===
    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);

    void start_and_return_();
    void HandleIngressPipeline();
    void Enqueue(uint32_t egress_port, std::unique_ptr<bm::Packet>&& packet) override;
    bool HandleEgressPipeline(size_t workerId) override;

    void CalculateScheduleTime();
    void CalculatePacketsPerSecond();

    void SetEgressTimerEvent();
    void MulticastPacket(bm::Packet* packet, unsigned int mgid);

    void CopyFieldList(const std::unique_ptr<bm::Packet>& packet,
                       const std::unique_ptr<bm::Packet>& packetCopy,
                       PktInstanceType copyType,
                       int fieldListId);

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

#endif // P4_CORE_V1MODEL_DEV_H
