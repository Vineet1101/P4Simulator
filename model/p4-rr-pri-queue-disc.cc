#include "ns3/p4-rr-pri-queue-disc.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/priority-port-tag.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NSP4PriQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (NSP4PriQueueDisc);

TypeId
NSP4PriQueueDisc::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::NSP4PriQueueDisc")
          .SetParent<QueueDisc> ()
          .SetGroupName ("TrafficControl")
          .AddConstructor<NSP4PriQueueDisc> ()
          .AddAttribute ("NumPorts", "Number of ports to manage.", UintegerValue (4),
                         MakeUintegerAccessor (&NSP4PriQueueDisc::m_nbPorts),
                         MakeUintegerChecker<uint8_t> ())
          .AddAttribute ("NumPriorities", "Number of priority queues per port.", UintegerValue (8),
                         MakeUintegerAccessor (&NSP4PriQueueDisc::m_nbPriorities),
                         MakeUintegerChecker<uint8_t> ());

  return tid;
}

NSP4PriQueueDisc::NSP4PriQueueDisc () : m_rng (CreateObject<UniformRandomVariable> ())
{
  NS_LOG_FUNCTION (this);

  m_priorityQueues.resize (m_nbPorts);
  for (auto &portQueues : m_priorityQueues)
    {
      portQueues.resize (m_nbPriorities);
    }

  NS_LOG_DEBUG ("NSP4PriQueueDisc created with " << m_nbPorts << " ports and " << m_nbPriorities
                                                 << " priorities.");
}

NSP4PriQueueDisc::~NSP4PriQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

uint32_t
NSP4PriQueueDisc::GetQueueSize (uint8_t port, uint8_t priority) const
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  return m_priorityQueues[port][priority].queue.size ();
}

uint32_t
NSP4PriQueueDisc::GetQueueCapacity (uint8_t port, uint8_t priority) const
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  return m_priorityQueues[port][priority].capacity;
}

uint64_t
NSP4PriQueueDisc::GetQueueRate (uint8_t port, uint8_t priority) const
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  return m_priorityQueues[port][priority].ratePps;
}

void
NSP4PriQueueDisc::SetQueueCapacity (uint8_t port, uint8_t priority, uint32_t capacity)
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  m_priorityQueues[port][priority].capacity = capacity;
}

void
NSP4PriQueueDisc::SetQueueRate (uint8_t port, uint8_t priority, uint64_t ratePps)
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  m_priorityQueues[port][priority].ratePps = ratePps;
  m_priorityQueues[port][priority].delayTime = RateToTime (ratePps);
}

uint32_t
NSP4PriQueueDisc::GetQueueTotalLengthPerPort (uint8_t port) const
{
  NS_ASSERT (port < m_nbPorts);
  uint32_t totalLength = 0;
  for (uint8_t priority = 0; priority < m_nbPriorities; ++priority)
    {
      totalLength += m_priorityQueues[port][priority].queue.size ();
    }
  return totalLength;
}

uint32_t
NSP4PriQueueDisc::GetVirtualQueueLengthPerPort (uint8_t port, uint8_t priority) const
{
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  return m_priorityQueues[port][priority].queue.size ();
}

bool
NSP4PriQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  Ptr<Packet> packet = item->GetPacket ();
  PriorityPortTag tag;

  if (packet->PeekPacketTag (tag))
    {
      uint8_t priority = tag.GetPriority () % m_nbPriorities;
      uint16_t port = tag.GetPort ();

      if (port >= m_nbPorts || priority >= m_nbPriorities)
        {
          NS_LOG_ERROR ("Invalid port or priority.");
          return false;
        }

      auto &fifoQueue = m_priorityQueues[port][priority];
      if (fifoQueue.queue.size () >= fifoQueue.capacity)
        {
          NS_LOG_WARN ("Queue overflow for port " << port << ", priority " << priority);
          DropBeforeEnqueue (item, "Overlimit drop");
          return false;
        }

      Time sendTime = CalculateNextSendTime (fifoQueue);

      fifoQueue.queue.push (
          Create<P4QueueItem> (item->GetPacket (), item->GetAddress (), item->GetProtocol ()));

      // set the send time for the packet
      fifoQueue.queue.back ()->SetSendTime (sendTime);

      NS_LOG_INFO ("Packet enqueued to port " << port << ", priority " << priority);
    }
  else
    {
      NS_LOG_WARN ("Packet missing PriorityPortTag.");
      return false;
    }

  return true;
}

Ptr<QueueDiscItem>
NSP4PriQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  for (uint8_t port = 0; port < m_nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < m_nbPriorities; ++priority)
        {
          auto &fifoQueue = m_priorityQueues[port][priority];
          if (fifoQueue.queue.empty ())
            {
              continue;
            }

          Time now = Simulator::Now ();
          if (fifoQueue.queue.front ()->GetSendTime () <= now)
            {
              Ptr<QueueDiscItem> item = fifoQueue.queue.front ();
              fifoQueue.queue.pop ();

              NS_LOG_INFO ("Packet dequeued from port " << port << ", priority " << priority);
              return item;
            }
        }
    }

  return nullptr;
}

Ptr<const QueueDiscItem>
NSP4PriQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  for (uint8_t port = 0; port < m_nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < m_nbPriorities; ++priority)
        {
          auto &fifoQueue = m_priorityQueues[port][priority];
          if (fifoQueue.queue.empty ())
            {
              continue;
            }

          Time now = Simulator::Now ();
          if (fifoQueue.queue.front ()->GetSendTime () <= now)
            {
              NS_LOG_INFO ("Packet peeked from port " << port << ", priority " << priority);
              return fifoQueue.queue.front ();
            }
        }
    }

  return nullptr;
}

bool
NSP4PriQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);

  if (m_priorityQueues.size () != m_nbPorts)
    {
      NS_LOG_ERROR ("Mismatch between ports and queue configuration.");
      return false;
    }

  for (const auto &portQueues : m_priorityQueues)
    {
      if (portQueues.size () != m_nbPriorities)
        {
          NS_LOG_ERROR ("Mismatch between priorities and queue configuration.");
          return false;
        }
    }

  return true;
}

Time
NSP4PriQueueDisc::CalculateNextSendTime (const FifoQueue &fifoQueue) const
{
  return Simulator::Now () + fifoQueue.delayTime;
}

Time
NSP4PriQueueDisc::RateToTime (uint64_t pps)
{
  return (pps == 0) ? MilliSeconds (1) : Seconds (1.0 / pps);
}

void
NSP4PriQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
