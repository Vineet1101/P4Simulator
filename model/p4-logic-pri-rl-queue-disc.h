#ifndef NS3_NS_QUEUEING_LOGIC_PRI_RL_QUEUE_DISC_H
#define NS3_NS_QUEUEING_LOGIC_PRI_RL_QUEUE_DISC_H

#include "ns3/packet.h"
#include "ns3/queue-disc.h"

#include <queue>
#include <vector>

namespace ns3
{

// Reasons for dropping packets
static constexpr const char* LIMIT_EXCEEDED_DROP = "Overlimit drop"; //!< Overlimit dropped packets

/**
 * \ingroup traffic-control
 * \brief A priority queue with rate limiting and delay
 *
 * Here we can reference QueueingLogicPriRL by include<bm_sim/queueing.h>, and the queuedisc can be
 * used as a queue, but this cannot accurately control the simulation time.
 *
 */
class NSQueueingLogicPriRLQueueDisc : public QueueDisc
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
    NSQueueingLogicPriRLQueueDisc();

    ~NSQueueingLogicPriRLQueueDisc();

    /**
     * \brief Get the queue lenght of the queue for a given priority
     *
     * \param priority the priority
     * \return queue lenght
     */
    uint32_t GetQueueSize(size_t priority) const;

    /**
     * \brief Get the capacity of the queue for a given priority
     *
     * \param priority the priority
     * \return the capacity of the queue [number of packets]
     */
    uint32_t GetQueueCapacity(size_t priority) const;

    /**
     * \brief Get the rate of the queue for a given priority
     *
     * \param priority the priority
     * \return rate[PPS], the rate of the queue
     */
    uint64_t GetQueueRate(size_t priority) const;

    /**
     * \brief Set the capacity of the queue for a given priority
     *
     * \param priority the priority
     * \param the capacity of the queue [number of packets]
     */
    void SetQueueCapacity(size_t priority, uint32_t capacity);

    /**
     * \brief Set the rate of the queue for a given priority
     *
     * \param priority the priority
     * \param rate[PPS], the rate of the queue
     */
    void SetQueueRate(size_t priority, uint64_t ratePps);

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

    struct FifoQueue
    {
        FifoQueue(size_t nbPorts)
            : queueLengthPort(nbPorts, 0)
        {
        }

        std::queue<QueueElement> queue;
        uint32_t size{0};
        uint32_t capacity{1000};
        uint64_t ratePps{1000};
        Time delayTime;
        std::vector<uint32_t> queueLengthPort; // queuelenght for each port
    };

    /**
     * \brief Calculate the next time to send a packet
     *
     * \param priorityQueue the priority queue
     * \return the next time to send a packet
     */
    Time CalculateNextSendTime(const FifoQueue& priorityQueue) const;

    /**
     * \brief Convert rate to time
     */
    static constexpr Time RateToTime(uint64_t pps);

    std::vector<FifoQueue> m_priorityQueues; // priority queues for all ports
    size_t m_nbPorts{32};                    // default max 32 ports for switch
    size_t m_nbPriorities{8};                // default 8 priorities <3 bit>
};

} // namespace ns3

#endif // NS3_NS_QUEUEING_LOGIC_PRI_RL_QUEUE_DISC_H
