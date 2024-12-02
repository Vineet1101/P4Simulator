#ifndef NS3_P4_RR_PRI_QUEUE_DISC_H
#define NS3_P4_RR_PRI_QUEUE_DISC_H

#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/random-variable-stream.h"

#include <queue>
#include <vector>

namespace ns3
{

// Reasons for dropping packets
// static constexpr const char* LIMIT_EXCEEDED_DROP = "Overlimit drop"; //!< Overlimit dropped packets

/**
 * \ingroup traffic-control
 * \brief A priority queue with rate limiting and delay
 *
 * Here we can reference QueueingLogicPriRL by include<bm_sim/queueing.h>, and the queuedisc can be
 * used as a queue, but this cannot accurately control the simulation time.
 *
 */
class NSP4PriQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * \brief P4 PrioQueueDisc constructor
     */
    NSP4PriQueueDisc();

    ~NSP4PriQueueDisc();

    /**
     * \brief Get the queue lenght of the queue for a given priority
     *
     * \param port the port
     * \param priority the priority
     *
     */
    uint32_t GetQueueSize(uint8_t port, uint8_t priority) const;

    /**
     * \brief Get the capacity of the queue for a given priority
     *
     * \param port the port
     * \param priority the priority
     * \return the capacity of the queue [number of packets]
     */
    uint32_t GetQueueCapacity(uint8_t port, uint8_t priority) const;

    /**
     * \brief Get the rate of the queue for a given priority
     *
     * \param port the port
     * \param priority the priority
     * \return rate[PPS], the rate of the queue
     */
    uint64_t GetQueueRate(uint8_t port, uint8_t priority) const;

    /**
     * \brief Set the capacity of the queue for a given priority
     *
     * \param port the port
     * \param priority the priority
     * \param the capacity of the queue [number of packets]
     */
    void SetQueueCapacity(uint8_t port, uint8_t priority, uint32_t capacity);

    /**
     * \brief Set the rate of the queue for a given priority
     *
     * \param port the port
     * \param priority the priority
     * \param rate[PPS], the rate of the queue
     */
    void SetQueueRate(uint8_t port, uint8_t priority, uint64_t ratePps);

  protected:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue(void) override;
    Ptr<const QueueDiscItem> DoPeek(void) const;
    bool CheckConfig(void) override;
    virtual void InitializeParams(void) override;

  private:
    /**
     * \brief QueueElement structure
     */
    struct QueueElement
    {
        Ptr<QueueDiscItem> item;
        Time sendTime;

        QueueElement(Ptr<QueueDiscItem> i, Time s)
            : item(i),
              sendTime(s)
        {
        }

        bool operator<(const QueueElement& other) const
        {
            return sendTime > other.sendTime;
        }
    };

    /**
     * \brief FifoQueue structure
     *
     */
    struct FifoQueue
    {
        std::queue<QueueElement> queue;
        uint32_t size{0};
        uint32_t capacity{1000};
        uint64_t ratePps{1000};
        Time delayTime;
        uint32_t queueLengthPort;
    };

    /**
     * \brief Calculate the next time to send a packet
     *
     * \param priorityQueue the priority queue
     * \return the next time to send a packet
     */
    Time CalculateNextSendTime(const FifoQueue& priorityQueue) const;

    /**
     * \brief Get the next port to send a packet
     *
     * \return the next port to dequeue a packet
     */
    uint8_t GetNextPort() const;

    /**
     * \brief Convert rate to time
     */
    static constexpr Time RateToTime(uint64_t pps);

    std::vector<std::vector<FifoQueue>> m_priorityQueues; // priority queues for all ports
    uint8_t m_nbPorts{4};                                 // default 4 ports for switch
    uint8_t m_nbPriorities{8};                            // default 8 priorities <3 bit>
    Ptr<UniformRandomVariable> m_rng;                     // Random number generator
};

} // namespace ns3

#endif // NS3_P4_RR_PRI_QUEUE_DISC_H
