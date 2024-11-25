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
    static TypeId GetTypeId(void);

    NSQueueingLogicPriRLQueueDisc();
    NSQueueingLogicPriRLQueueDisc(size_t nbPorts, size_t nbPriorities);
    virtual ~NSQueueingLogicPriRLQueueDisc();

    // void SetNbPorts (size_t nbPorts);
    // void SetNbPriorities(size_t nbPriorities);
    void SetQueueCapacity(size_t priority, uint32_t capacity);
    void SetQueueRate(size_t priority, uint64_t ratePps);

  protected:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    // bool DoEnqueue (Ptr<QueueDiscItem> item, size_t port);
    
    Ptr<QueueDiscItem> DoDequeue(void) override;
    virtual Ptr<QueueDiscItem> DoDequeue(size_t port);

    virtual Ptr<const QueueDiscItem> DoPeek(void) const;
    virtual bool CheckConfig(void) override;

  private:
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
            {}

        std::queue<QueueElement> queue;
        uint32_t size{0};
        uint32_t capacity{1000};
        uint64_t ratePps{1000};
        Time delayTime;
        std::vector<uint32_t> queueLengthPort; // queuelenght for each port
    };

    // std::vector<std::vector<PriorityQueue>> m_portPriorityQueues; // priority queues for each port
    std::vector<FifoQueue> m_priorityQueues; // priority queues for all ports
    size_t m_nbPorts{32}; // default max 32 ports for switch
    size_t m_nbPriorities{8}; // default 8 priorities <3 bit>

    Time CalculateNextSendTime(const FifoQueue& priorityQueue) const;
    static constexpr Time RateToTime(uint64_t pps);
};

} // namespace ns3

#endif // NS3_NS_QUEUEING_LOGIC_PRI_RL_QUEUE_DISC_H
