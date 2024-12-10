#include "p4-queue.h"
#include "ns3/log.h"
#include "ns3/double.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4Queuebuffer");

P4Queuebuffer::P4Queuebuffer (uint32_t nPorts, uint32_t nPriorities)
    : m_nPorts (nPorts), m_nPriorities (nPriorities)
{
  NS_LOG_FUNCTION (this);

  // Initialize the random number generator
  // m_random.SetSeed (1);
  // Create a uniformly distributed random number generator range to [0, m_nbPorts - 1]
  m_rng = CreateObject<UniformRandomVariable> ();
  m_rng->SetAttribute ("Min", DoubleValue (0));
  m_rng->SetAttribute ("Max", DoubleValue (m_nPorts - 1));
  NS_LOG_INFO ("Uniform random variable created with Min = 0, Max = " << m_nPorts - 1);

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
P4Queuebuffer::SetRandomSeed (uint32_t seed)
{
  NS_LOG_FUNCTION (this << seed);
  m_rng->SetStream (seed);
  return true;
}

bool
P4Queuebuffer::Enqueue (Ptr<P4QueueItem> item)
{
  NS_LOG_FUNCTION (this << item);

  // Validate the port and priority
  uint32_t priority = item->GetMetadata ()->priority;
  uint32_t port = item->GetMetadata ()->egress_port;

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

uint32_t
P4Queuebuffer::GetQueueTotalLengthPerPort (uint8_t port) const
{
  NS_ASSERT (port < m_nPorts);
  uint32_t nQueued = m_queues.at (port).size ();
  return nQueued;
}

uint32_t
P4Queuebuffer::GetVirtualQueueLengthPerPort (uint8_t port, uint8_t priority) const
{
  // find the port and priority queue length
  NS_ASSERT (port < m_nPorts && priority < m_nPriorities);
  uint32_t nQueued = m_queues.at (port).at (priority).size ();
  return nQueued;
}

Ptr<P4QueueItem>
P4Queuebuffer::Dequeue ()
{
  if (is_peek_marked)
    {
      // avoid the peek packet different with the dequeue packet(in effect with random number gene).
      is_peek_marked = false;
      return Dequeue (marked_port);
    }
  else
    {
      uint32_t port = m_rng->GetInteger (0, m_nPorts - 1);
      NS_LOG_INFO ("Random port selected: " << port);

      std::set<uint32_t> triedPorts;
      while (triedPorts.size () < m_nPorts)
        {
          // Check whether the current port has been traversed
          if (triedPorts.find (port) == triedPorts.end ())
            {
              if (IsEmpty (port))
                {
                  NS_LOG_INFO ("Port " << port << " is empty. Trying another port...");
                  triedPorts.insert (port); // Log the ports that were tried
                  // Choose a new random port
                  port = m_rng->GetInteger (0, m_nPorts - 1);
                  NS_LOG_INFO ("choose new port because empty: " << port);
                }
              else
                {
                  NS_LOG_INFO ("Packet dequeued from port " << port);
                  return Dequeue (port); // If it is not empty, return directly
                }
            }
          else
            {
              // Choose a new random port
              port = m_rng->GetInteger (0, m_nPorts - 1);
              NS_LOG_INFO ("choose new port because tried: " << port);
            }
        }
      // If all ports have been tried and are empty, return nullptr
      NS_LOG_WARN ("All ports are empty. No packet dequeued.");
      return nullptr;
    }
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
  for (uint32_t priority = m_nPriorities - 1; priority >= 0; priority--)
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

Ptr<P4QueueItem>
P4Queuebuffer::Peek (uint32_t port)
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
          // m_queues[port][priority].pop ();
          NS_LOG_DEBUG ("Dequeued packet from port " << port << ", priority " << priority);
          return item;
        }
    }

  NS_LOG_DEBUG ("No packets available to dequeue at port " << port);
  return nullptr; // Queue is empty
}

Ptr<P4QueueItem>
P4Queuebuffer::Peek ()
{
  NS_LOG_FUNCTION (this);
  if (is_peek_marked)
    {
      // avoid the peek packet different with the dequeue packet(in effect with random number gene).
      is_peek_marked = false;
      return Peek (marked_port);
    }
  else
    {
      uint32_t port = m_rng->GetInteger (0, m_nPorts - 1);
      std::set<uint32_t> triedPorts;
      while (triedPorts.size () < m_nPorts)
        {
          // Check whether the current port has been traversed
          if (triedPorts.find (port) == triedPorts.end ())
            {
              triedPorts.insert (port); // Log the ports that were tried

              if (IsEmpty (port))
                {
                  NS_LOG_INFO ("Port " << port << " is empty. Trying another port...");
                  // Choose a new random port
                  port = m_rng->GetInteger (0, m_nPorts - 1);
                }
              else
                {
                  NS_LOG_INFO ("Packet peeked from port " << port);
                  return Peek (port); // If it is not empty, return directly
                }
            }
          else
            {
              // Choose a new random port
              port = m_rng->GetInteger (0, m_nPorts - 1);
            }
        }
      // If all ports have been tried and are empty, return nullptr
      NS_LOG_WARN ("All ports are empty. No packet peeked.");
      return nullptr;
    }
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

void
TwoTierP4Queue::Initialize ()
{
  NS_LOG_FUNCTION (this);
}

bool
TwoTierP4Queue::Enqueue (Ptr<P4QueueItem> item)
{
  NS_LOG_FUNCTION (this << item);

  // Check the PacketType from the metadata
  PacketType type = item->GetPacketType ();
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
