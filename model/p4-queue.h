#ifndef P4_QUEUE_H
#define P4_QUEUE_H

#include <map>
#include <queue>
#include "p4-queue-item.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"

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
   * \return True if enqueued successfully, false otherwise
   */
  bool Enqueue (Ptr<P4QueueItem> item);

  /**
   * \brief Dequeue a packet from the specified port and priority queue
   * \return The dequeued item, or nullptr if the queue is empty
   */
  Ptr<P4QueueItem> Dequeue ();

  /**
   * \brief Dequeue a packet from the specified port and priority queue
   * \param port The target port
   * \return The dequeued item, or nullptr if the queue is empty
   */
  Ptr<P4QueueItem> Dequeue (uint32_t port);

  /**
   * \brief Set the random seed for the queue
   * \param seed The random seed to set
   * \return True if the seed is set successfully, false otherwise
   */
  bool SetRandomSeed (uint32_t seed);

  /**
   * \brief Peek at the packet at the front of the specified port and priority queue
   * \param port The target port
   * \return The peeked item, or nullptr if the queue is empty
   */
  Ptr<P4QueueItem> Peek (uint32_t port);

  /**
   * \brief Peek at the packet at the front of the specified port and priority queue
   * \return The peeked item, or nullptr if the queue is empty
   */
  Ptr<P4QueueItem> Peek ();

  /**
   * \brief Check if the queue is empty for a specific port
   * \param port The port to check
   * \return True if the queue is empty, false otherwise
   */
  bool IsEmpty (uint32_t port) const;

  /**
   * \brief Get the total length of all queues for a specific port
   * \param port The port to check
   * \return The total length of all queues for the port
   */
  uint32_t GetQueueTotalLengthPerPort (uint8_t port) const;

  /**
   * \brief Get the virtual queue length for a specific port and priority
   * \param port The port to check
   * \param priority The priority to check
   * \return The virtual queue length for the port and priority
   */
  uint32_t GetVirtualQueueLengthPerPort (uint8_t port, uint8_t priority) const;

  /**
   * \brief Reset the virtual queue number, because the number of queues is changed
   * 
   * \return True if the change the port number, false otherwise
   */
  bool ReSetVirtualQueueNumber (uint32_t newSize);

  bool AddVirtualQueue (uint32_t port_num);

  bool RemoveVirtualQueue (uint32_t port_num);

private:
  uint32_t m_nPorts; //!< Number of ports
  uint32_t m_nPriorities; //!< Number of priorities per port
  Ptr<UniformRandomVariable> m_rng; //!< Random number generator

  bool is_peek_marked{false}; //!< Mark the peek packet
  uint32_t marked_port{0}; //!< Port of the marked packet

  // Map of ports to priority queues
  std::map<uint32_t, std::vector<std::queue<Ptr<P4QueueItem>>>> m_queues;
};

/**
 * \brief A two-tier priority queue for P4QueueItem. This is used for the Input
 * Queue Buffer (before Parser) in the P4SwitchCore model.
 * 
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

  void Initialize ();

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
