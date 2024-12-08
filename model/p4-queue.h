#ifndef P4_QUEUE_H
#define P4_QUEUE_H

#include <map>
#include <queue>
#include "p4-queue-item.h"
#include "ns3/object.h"

namespace ns3 {

/**
 * \brief Multi-port P4Queuebuffer with priority-based virtual queues
 */
class P4Queuebuffer : public Object
{
public:
  /**
   * \brief Constructor
   * \param nPorts Number of ports
   * \param nPriorities Number of priorities per port
   */
  P4Queuebuffer (uint32_t nPorts, uint32_t nPriorities);

  /**
   * \brief Destructor
   */
  virtual ~P4Queuebuffer ();

  /**
   * \brief Enqueue a packet into the appropriate port and priority queue
   * \param item The queue item to enqueue
   * \param port The target port
   * \return True if enqueued successfully, false otherwise
   */
  bool Enqueue (Ptr<P4QueueItem> item, uint32_t port);

  /**
   * \brief Dequeue a packet from the specified port and priority queue
   * \param port The target port
   * \return The dequeued item, or nullptr if the queue is empty
   */
  Ptr<P4QueueItem> Dequeue (uint32_t port);

  /**
   * \brief Check if the queue is empty for a specific port
   * \param port The port to check
   * \return True if the queue is empty, false otherwise
   */
  bool IsEmpty (uint32_t port) const;

private:
  uint32_t m_nPorts; //!< Number of ports
  uint32_t m_nPriorities; //!< Number of priorities per port

  // Map of ports to priority queues
  std::map<uint32_t, std::vector<std::queue<Ptr<P4QueueItem>>>> m_queues;
};

/**
 * \brief A two-tier priority queue for P4QueueItem
 */
class TwoTierP4Queue : public Object
{
public:
  /**
   * \brief Constructor
   */
  TwoTierP4Queue ();

  /**
   * \brief Destructor
   */
  virtual ~TwoTierP4Queue ();

  /**
   * \brief Enqueue a packet into the appropriate queue
   * \param item The queue item to enqueue
   * \return True if enqueued successfully, false otherwise
   */
  bool Enqueue (Ptr<P4QueueItem> item);

  /**
   * \brief Dequeue a packet from the queues (high priority first)
   * \return The dequeued item, or nullptr if both queues are empty
   */
  Ptr<P4QueueItem> Dequeue ();

  /**
   * \brief Check if both queues are empty
   * \return True if both queues are empty, false otherwise
   */
  bool IsEmpty () const;

private:
  std::queue<Ptr<P4QueueItem>> m_highPriorityQueue; //!< High-priority queue
  std::queue<Ptr<P4QueueItem>> m_lowPriorityQueue; //!< Low-priority queue
};

} // namespace ns3

#endif /* P4_QUEUE_H */
