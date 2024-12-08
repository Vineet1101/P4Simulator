#include "p4-queue.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4Queuebuffer");

P4Queuebuffer::P4Queuebuffer (uint32_t nPorts, uint32_t nPriorities)
    : m_nPorts (nPorts), m_nPriorities (nPriorities)
{
  NS_LOG_FUNCTION (this);

  // Initialize the queues for each port and priority
  for (uint32_t port = 0; port < m_nPorts; ++port)
    {
      std::vector<std::queue<Ptr<P4QueueItem>>> portQueues (m_nPriorities);
      m_queues[port] = portQueues;
    }
}

P4Queuebuffer::~P4Queuebuffer ()
{
  NS_LOG_FUNCTION (this);
}

bool
P4Queuebuffer::Enqueue (Ptr<P4QueueItem> item, uint32_t port)
{
  NS_LOG_FUNCTION (this << item << port);

  // Validate the port and priority
  uint32_t priority = item->GetMetadata ().priority;
  if (port >= m_nPorts || priority >= m_nPriorities)
    {
      NS_LOG_ERROR ("Invalid port or priority");
      return false;
    }

  // Enqueue the item into the appropriate queue
  m_queues[port][priority].push (item);
  NS_LOG_DEBUG ("Enqueued packet at port " << port << ", priority " << priority);
  return true;
}

Ptr<P4QueueItem>
P4Queuebuffer::Dequeue (uint32_t port)
{
  NS_LOG_FUNCTION (this << port);

  if (port >= m_nPorts)
    {
      NS_LOG_ERROR ("Invalid port");
      return nullptr;
    }

  // Dequeue from the highest-priority non-empty queue
  for (uint32_t priority = 0; priority < m_nPriorities; ++priority)
    {
      if (!m_queues[port][priority].empty ())
        {
          Ptr<P4QueueItem> item = m_queues[port][priority].front ();
          m_queues[port][priority].pop ();
          NS_LOG_DEBUG ("Dequeued packet from port " << port << ", priority " << priority);
          return item;
        }
    }

  NS_LOG_DEBUG ("No packets available to dequeue at port " << port);
  return nullptr; // Queue is empty
}

bool
P4Queuebuffer::IsEmpty (uint32_t port) const
{
  NS_LOG_FUNCTION (this << port);

  if (port >= m_nPorts)
    {
      NS_LOG_ERROR ("Invalid port");
      return true;
    }

  // Check all priority queues for the specified port
  for (uint32_t priority = 0; priority < m_nPriorities; ++priority)
    {
      if (!m_queues.at (port).at (priority).empty ())
        {
          return false;
        }
    }

  return true;
}

TwoTierP4Queue::TwoTierP4Queue ()
{
  NS_LOG_FUNCTION (this);
}

TwoTierP4Queue::~TwoTierP4Queue ()
{
  NS_LOG_FUNCTION (this);
}

bool
TwoTierP4Queue::Enqueue (Ptr<P4QueueItem> item)
{
  NS_LOG_FUNCTION (this << item);

  // Check the PacketType from the metadata
  PacketType type = item->GetMetadata ().type;
  if (type == PacketType::NORMAL)
    {
      m_lowPriorityQueue.push (item);
      NS_LOG_DEBUG ("Packet enqueued to low-priority queue.");
    }
  else
    {
      m_highPriorityQueue.push (item);
      NS_LOG_DEBUG ("Packet enqueued to high-priority queue.");
    }

  return true;
}

Ptr<P4QueueItem>
TwoTierP4Queue::Dequeue ()
{
  NS_LOG_FUNCTION (this);

  // Check and dequeue from the high-priority queue first
  if (!m_highPriorityQueue.empty ())
    {
      Ptr<P4QueueItem> item = m_highPriorityQueue.front ();
      m_highPriorityQueue.pop ();
      NS_LOG_DEBUG ("Dequeued packet from high-priority queue.");
      return item;
    }

  // If high-priority queue is empty, dequeue from low-priority queue
  if (!m_lowPriorityQueue.empty ())
    {
      Ptr<P4QueueItem> item = m_lowPriorityQueue.front ();
      m_lowPriorityQueue.pop ();
      NS_LOG_DEBUG ("Dequeued packet from low-priority queue.");
      return item;
    }

  // Both queues are empty
  NS_LOG_DEBUG ("Both queues are empty.");
  return nullptr;
}

bool
TwoTierP4Queue::IsEmpty () const
{
  NS_LOG_FUNCTION (this);
  return m_highPriorityQueue.empty () && m_lowPriorityQueue.empty ();
}

} // namespace ns3
