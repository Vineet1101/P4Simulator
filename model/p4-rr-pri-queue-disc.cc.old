/**
 * 【Deprecated 弃用】
 * 
 * 这种实现不够灵活，p4switch基于bridge实现，而通常的queue-disc无法自行构建queue-item，而通常不自行实现queue-item。
 * 在一定的探索和思考后：
 * 我认为可以模仿<wifi-mac-queue.h>的实现，分别实现队列和队列项，更加灵活。
 * 这样可以更好地控制队列的行为以及存储p4中每个packet的metadata
 * （当然，如果可行写一个packet类，内含metadata更好，但是这样工作量会非常高，同时面临大量的重复工作）
 * 而我们观测到，在switch中，packet不是bm::packet就是ns::packet，前者是使用bmv2的运算和处理，后者主要是作为排队，而
 * 排队使用queue-item，此时我们只需将bm::packet的metadata存储在queue-item中，仍可避免复杂化，以及保证高效率。
 * 
 * The current implementation lacks flexibility: the p4switch is based on the bridge module, 
 * and the typical queue-disc cannot construct its own queue-items independently, nor is it common 
 * to implement custom queue-items.
 *
 * After some exploration and consideration:
 * I believe we can model the implementation after <wifi-mac-queue.h>, 
 * separately implementing the queue and queue-item for greater flexibility. 
 * This approach allows better control over the queue's behavior and the storage of metadata 
 * for each packet processed in the P4 pipeline.
 *
 * (Of course, if feasible, creating a Packet class containing metadata would be ideal. 
 * However, this would significantly increase workload and introduce substantial redundancy.)
 *
 * Observing the current behavior:
 * In the switch, a packet is either a `bm::packet` (used for bmv2's computation and processing) 
 * or an `ns3::Packet` (primarily used for queuing). 
 * Queuing relies on `queue-item`, so we only need to store the metadata from `bm::packet` 
 * in the `queue-item`. This approach avoids unnecessary complexity while maintaining high efficiency.
 */

#include "ns3/p4-rr-pri-queue-disc.h"

#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/priority-port-tag.h"
#include "ns3/traffic-control-module.h"
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
}

NSP4PriQueueDisc::~NSP4PriQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

// uint32_t
// NSP4PriQueueDisc::GetQueueSize (uint8_t port, uint8_t priority) const
// {
//   NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
//   return m_priorityQueues[port][priority].queue.size ();
// }

// uint32_t
// NSP4PriQueueDisc::GetQueueCapacity (uint8_t port, uint8_t priority) const
// {
//   NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
//   return m_priorityQueues[port][priority].capacity;
// }

// uint64_t
// NSP4PriQueueDisc::GetQueueRate (uint8_t port, uint8_t priority) const
// {
//   NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
//   return m_priorityQueues[port][priority].ratePps;
// }

// void
// NSP4PriQueueDisc::SetQueueCapacity (uint8_t port, uint8_t priority, uint32_t capacity)
// {
//   NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
//   m_priorityQueues[port][priority].capacity = capacity;
// }

// void
// NSP4PriQueueDisc::SetQueueRate (uint8_t port, uint8_t priority, uint64_t ratePps)
// {
//   NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
//   m_priorityQueues[port][priority].ratePps = ratePps;
//   m_priorityQueues[port][priority].delayTime = RateToTime (ratePps);
// }

uint32_t
NSP4PriQueueDisc::GetQueueTotalLengthPerPort (uint8_t port) const
{
  NS_ASSERT (port < m_nbPorts);
  // find the port queue length
  uint32_t nQueued = GetQueueDiscClass (port)->GetQueueDisc ()->GetNPackets ();
  return nQueued;
}

uint32_t
NSP4PriQueueDisc::GetVirtualQueueLengthPerPort (uint8_t port, uint8_t priority) const
{
  // find the port and priority queue length
  NS_ASSERT (port < m_nbPorts && priority < m_nbPriorities);
  uint32_t nQueued = GetQueueDiscClass (port)
                         ->GetQueueDisc ()
                         ->GetQueueDiscClass (priority)
                         ->GetQueueDisc ()
                         ->GetNPackets ();
  return nQueued;
}

uint32_t
NSP4PriQueueDisc::GetNextPort () const
{
  // 创建一个均匀分布的随机数生成器
  return m_rng->GetInteger ();
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
      NS_ASSERT_MSG (priority < GetNQueueDiscClasses (), "Selected priority out of range");
      uint16_t port = tag.GetPort ();

      if (port >= m_nbPorts || priority >= m_nbPriorities)
        {
          NS_LOG_ERROR ("Invalid port or priority.");
          return false;
        }

      // Add new packet tag for the packet in item
      packet->RemovePacketTag (tag);
      SocketPriorityTag priorityTag;
      priorityTag.SetPriority (priority);
      packet->AddPacketTag (priorityTag);

      // @TODO Test the tag
      bool retval = GetQueueDiscClass (port)->GetQueueDisc ()->Enqueue (item);

      // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue disc
      // because QueueDisc::AddQueueDiscClass sets the drop callback

      NS_LOG_INFO ("Packet enqueued to port " << port << ", priority " << priority);
      return retval;
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

  if (is_peek_marked)
    {
      // avoid the peek packet different with the dequeue packet(in effect with random number gene).
      is_peek_marked = false;
      return GetQueueDiscClass (marked_port)->GetQueueDisc ()->Dequeue ();
    }
  else
    {
      uint32_t port = GetNextPort ();
      std::set<uint32_t> triedPorts;
      while (triedPorts.size () < m_nbPorts)
        {
          Ptr<QueueDiscItem> item = GetQueueDiscClass (port)->GetQueueDisc ()->Dequeue ();
          if (item)
            {
              NS_LOG_INFO ("Packet dequeued from port " << port);
              return item;
            }

          NS_LOG_INFO ("Port " << port << " is empty. Trying another port...");

          //
          triedPorts.insert (port);

          // Generate a new random port, making sure it has not been tried before
          do
            {
              port = GetNextPort ();
          } while (triedPorts.count (port) > 0);
        }
    }
  // If all ports have been tried and are empty, return nullptr
  NS_LOG_WARN ("All ports are empty. No packet dequeued.");
  return nullptr;
}

Ptr<const QueueDiscItem>
NSP4PriQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  if (is_peek_marked)
    {
      // re-peek again
      return GetQueueDiscClass (marked_port)->GetQueueDisc ()->Peek ();
    }
  else
    {
      uint32_t port = GetNextPort ();
      std::set<uint32_t> triedPorts;
      Ptr<const QueueDiscItem> item;
      while (triedPorts.size () < m_nbPorts)
        {
          item = GetQueueDiscClass (port)->GetQueueDisc ()->Peek ();
          if (item)
            {
              // save the peek port and mark the peeked packet
              is_peek_marked = true;
              marked_port = port;
              NS_LOG_INFO ("Packet peeked from port " << port);
              return item;
            }

          NS_LOG_INFO ("Port " << port << " is empty. Trying another port...");

          //
          triedPorts.insert (port);

          // Generate a new random port, making sure it has not been tried before
          do
            {
              port = GetNextPort ();
          } while (triedPorts.count (port) > 0);
        }
    }
  // If all ports have been tried and are empty, return nullptr
  NS_LOG_WARN ("All ports are empty. No packet peeked.");
  return nullptr;
}

bool
NSP4PriQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);

  // implement the queue, check the configuration
  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("P4QueueDisc cannot have internal queues");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("P4QueueDisc cannot have any packet filters");
      return false;
    }

  int maxVirtualQueues = 8; // 3 bits mark for the priority
  if (m_nbPriorities > maxVirtualQueues)
    {
      NS_LOG_ERROR ("Number of priorities must be less than 8 (3 bits).");
      return false;
    }

  // Initialize the priority queues
  if (GetNQueueDiscClasses () == 0)
    {
      // create m_nbPorts priority queues
      // m_nbPriorities for each priority queue discs
      ObjectFactory factory;
      factory.SetTypeId ("ns3::PrioQueueDisc");

      const std::array<uint16_t, 16> priority_map = {0, 1, 2, 3, 4, 5, 6, 7,
                                                     0, 0, 0, 0, 0, 0, 0, 0};
      // Config::SetDefault ("ns3::PrioQueueDisc::Priomap", PriomapValue (priority_map));

      for (uint8_t i = 0; i < m_nbPorts; i++)
        {
          Ptr<QueueDisc> qd = factory.Create<QueueDisc> ();
          qd->SetAttribute ("Priomap", PriomapValue (priority_map));
          qd->Initialize ();
          Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass> ();
          c->SetQueueDisc (qd);
          AddQueueDiscClass (c);
        }
    }

  if (GetNQueueDiscClasses () != m_nbPriorities)
    {
      NS_LOG_ERROR ("P4QueueDisc requires exact " << m_nbPriorities << " class");
      return false;
    }

  // // Check if timer events should be scheduled
  // if (!m_timeReference.IsZero ())
  //   {
  //     NS_LOG_DEBUG ("Scheduling initial timer event using m_timeReference = "
  //                   << m_timeReference.GetNanoSeconds () << " ns");
  //     m_timerEvent = Simulator::Schedule (m_timeReference, &P4QueueDisc::RunTimerEvent, this);
  //   }

  // // Check if drop events are enabled
  // if (m_enDropEvents)
  //   {
  //     TraceConnectWithoutContext ("DropBeforeEnqueue",
  //                                 MakeCallback (&P4QueueDisc::RunDropEvent, this));
  //   }

  // // Check if enqueue events are enabled
  // if (m_enEnqEvents)
  //   {
  //     TraceConnectWithoutContext ("Enqueue", MakeCallback (&P4QueueDisc::RunEnqEvent, this));
  //   }

  // // Check if dequeue events are enabled
  // if (m_enDeqEvents)
  //   {
  //     TraceConnectWithoutContext ("Dequeue", MakeCallback (&P4QueueDisc::RunDeqEvent, this));
  //   }

  return true;
}

void
NSP4PriQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  // You can set the global seed before use (if you need to control the random number generation process)

  // Create a uniformly distributed random number generator range to [0, m_nbPorts - 1]
  m_rng = CreateObject<UniformRandomVariable> ();
  m_rng->SetAttribute ("Min", DoubleValue (0));
  m_rng->SetAttribute ("Max", DoubleValue (m_nbPorts - 1));
  NS_LOG_INFO ("Uniform random variable created with Min = 0, Max = " << m_nbPorts - 1);
}

} // namespace ns3
