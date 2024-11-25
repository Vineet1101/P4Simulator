#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "p4-logic-pri-rl-queue-disc.h"

#include "priority-port-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NSQueueingLogicPriRLQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (NSQueueingLogicPriRLQueueDisc);

TypeId
NSQueueingLogicPriRLQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NSQueueingLogicPriRLQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<NSQueueingLogicPriRLQueueDisc> ();
  return tid;
}

NSQueueingLogicPriRLQueueDisc::NSQueueingLogicPriRLQueueDisc ()
{
  // init the priority queues with port vector
  m_priorityQueues.reserve(m_nbPriorities);
  for (size_t i = 0; i < m_nbPriorities; ++i) {
      m_priorityQueues.emplace_back(m_nbPorts);
  }

  NS_LOG_DEBUG ("NSQueueingLogicPriRLQueueDisc default constructor with " << m_nbPorts<<" ports and "<< m_nbPriorities << " priorities");
}

NSQueueingLogicPriRLQueueDisc::~NSQueueingLogicPriRLQueueDisc ()
{
}

void
NSQueueingLogicPriRLQueueDisc::SetQueueCapacity (size_t priority, uint32_t capacity)
{
  if (priority >= m_nbPriorities)
    {
      NS_LOG_ERROR ("Invalid priority specified in SetQueueCapacity");
      return;
    }
  m_priorityQueues[priority].capacity = capacity;
}

void
NSQueueingLogicPriRLQueueDisc::SetQueueRate (size_t priority, uint64_t ratePps)
{
  if (priority >= m_nbPriorities)
    {
      NS_LOG_ERROR ("Invalid priority specified in SetQueueRate");
      return;
    }
  m_priorityQueues[priority].ratePps = ratePps;
  m_priorityQueues[priority].delayTime = RateToTime (ratePps);
}


bool 
NSQueueingLogicPriRLQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  Ptr<Packet> packet = item->GetPacket ();
  PriorityPortTag tag;

  if (packet->PeekPacketTag(tag))
  {
    uint8_t priority = tag.GetPriority() % m_nbPriorities;
    uint16_t port = tag.GetPort();

    if (port >= m_nbPorts || priority >= m_nbPriorities)
    {
      NS_LOG_ERROR ("Invalid port or priority specified in DoEnqueue");
      return false;
    }

    auto &priorityQueue = m_priorityQueues[priority];
    if (priorityQueue.size >= priorityQueue.capacity)
      {
        NS_LOG_WARN ("Queue overflow for port " << port << ", priority " << priority);
        DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
        return false;
      }

    Time sendTime = CalculateNextSendTime (priorityQueue);
    priorityQueue.queue.emplace (std::move (item), sendTime);
    priorityQueue.size++;
    priorityQueue.queueLengthPort[port]++;

    NS_LOG_INFO("Packet contains a PriorityPortTag with priority " << priority << " and port " << port);

  }
  else
  {
      NS_LOG_WARN("Packet does not contain a PriorityPortTag");
  }
  return true;
}


Ptr<QueueDiscItem>
NSQueueingLogicPriRLQueueDisc::DoDequeue (void)
{
  for (size_t pri = 0; pri < m_nbPriorities; pri++)
    {
      auto &fifoQueue = m_priorityQueues[pri];
      if (fifoQueue.queue.empty ())
        {
          continue;
        }

      Time now = Simulator::Now ();
      if (fifoQueue.queue.front ().sendTime <= now)
        {
          Ptr<QueueDiscItem> dequeuedItem = fifoQueue.queue.front ().item;
          fifoQueue.queue.pop ();
          fifoQueue.size--;
          return dequeuedItem;
        }
    }
  return nullptr;
}

Ptr<QueueDiscItem>
NSQueueingLogicPriRLQueueDisc::DoDequeue (size_t port)
{
  for (size_t pri = 0; pri < m_nbPriorities; pri++)
    {
      auto &priorityQueue = m_priorityQueues[pri];
      if (priorityQueue.queue.empty ())
        {
          continue;
        }

      Time now = Simulator::Now ();
      if (priorityQueue.queue.front ().sendTime <= now)
        {
          Ptr<QueueDiscItem> dequeuedItem = priorityQueue.queue.front ().item;
          priorityQueue.queue.pop ();
          priorityQueue.size--;
          return dequeuedItem;
        }
    }

  return nullptr;
}


Ptr<const QueueDiscItem>
NSQueueingLogicPriRLQueueDisc::DoPeek (void) const
{
  for (size_t port = 0; port < m_nbPorts; port++)
    {
      for (size_t pri = 0; pri < m_nbPriorities; pri++)
        {
          const auto &priorityQueue = m_priorityQueues[pri];
          if (!priorityQueue.queue.empty () &&
              priorityQueue.queue.front ().sendTime <= Simulator::Now ())
            {
              return priorityQueue.queue.front ().item;
            }
        }
    }
  return nullptr;
}

bool
NSQueueingLogicPriRLQueueDisc::CheckConfig (void)
{
  if (m_priorityQueues.size () != m_nbPriorities)
    {
      NS_LOG_ERROR ("Number of priority queues for each port does not match the set number of priorities");
      return false;
    }
  return true;
}

Time
NSQueueingLogicPriRLQueueDisc::CalculateNextSendTime (const FifoQueue &FifoQueue) const
{
  Time now = Simulator::Now ();
  return now + FifoQueue.delayTime;
}

constexpr Time
NSQueueingLogicPriRLQueueDisc::RateToTime (uint64_t pps)
{
  return (pps == 0) ? MilliSeconds (1) : Seconds (1.0 / pps);
}

} // namespace ns3
