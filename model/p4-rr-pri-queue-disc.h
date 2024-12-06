#ifndef NS3_P4_RR_PRI_QUEUE_DISC_H
#define NS3_P4_RR_PRI_QUEUE_DISC_H

#include "ns3/p4-queue-item.h"
#include "ns3/queue-disc.h"
#include "ns3/random-variable-stream.h"

#include <queue>
#include <vector>

namespace ns3 {

/**
 * \ingroup traffic-control
 * \brief A round-robin priority queue with rate limiting and delay.
 *
 * This queue disc supports multiple ports, each with a set of priority queues.
 * Each priority queue is managed as a FIFO queue, and the dequeue operation
 * considers both priority and a round-robin scheduling across ports.
 */
class NSP4PriQueueDisc : public QueueDisc
{
public:
  /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
  static TypeId GetTypeId (void);

  /**
     * \brief Constructor
     */
  NSP4PriQueueDisc ();

  /**
     * \brief Destructor
     */
  ~NSP4PriQueueDisc ();

  /**
     * \brief Get the queue length for a given priority and port.
     *
     * \param port the port index
     * \param priority the priority index
     * \return the queue length
     */
  uint32_t GetQueueSize (uint8_t port, uint8_t priority) const;

  /**
     * \brief Get the capacity of the queue for a given priority and port.
     *
     * \param port the port index
     * \param priority the priority index
     * \return the queue capacity (number of packets)
     */
  uint32_t GetQueueCapacity (uint8_t port, uint8_t priority) const;

  /**
     * \brief Get the rate of the queue for a given priority and port.
     *
     * \param port the port index
     * \param priority the priority index
     * \return the rate in packets per second (PPS)
     */
  uint64_t GetQueueRate (uint8_t port, uint8_t priority) const;

  /**
     * \brief Set the capacity of the queue for a given priority and port.
     *
     * \param port the port index
     * \param priority the priority index
     * \param capacity the capacity (number of packets)
     */
  void SetQueueCapacity (uint8_t port, uint8_t priority, uint32_t capacity);

  /**
     * \brief Set the rate of the queue for a given priority and port.
     *
     * \param port the port index
     * \param priority the priority index
     * \param ratePps the rate in packets per second (PPS)
     */
  void SetQueueRate (uint8_t port, uint8_t priority, uint64_t ratePps);

  uint32_t GetQueueTotalLengthPerPort (uint8_t port) const;

  uint32_t GetVirtualQueueLengthPerPort (uint8_t port, uint8_t priority) const;

protected:
  bool DoEnqueue (Ptr<QueueDiscItem> item) override;
  Ptr<QueueDiscItem> DoDequeue (void) override;
  Ptr<const QueueDiscItem> DoPeek (void) override;
  bool CheckConfig (void) override;
  void InitializeParams (void) override;

private:
  /**
     * \brief FifoQueue structure to represent a single priority queue.
     */
  struct FifoQueue
  {
    std::queue<Ptr<P4QueueItem>> queue; //!< FIFO queue of packets
    uint32_t capacity{1000}; //!< Maximum capacity in packets
    uint64_t ratePps{1000}; //!< Rate in packets per second
    Time delayTime{MilliSeconds (1)}; //!< Delay time per packet
  };

  /**
     * \brief Calculate the next eligible send time for a packet in the queue.
     *
     * \param queue the priority queue
     * \return the calculated next send time
     */
  Time CalculateNextSendTime (const FifoQueue &queue) const;

  /**
     * \brief Find the next port and priority with a packet ready to dequeue.
     *
     * \return a pair containing the port index and priority index
     */
  std::pair<uint8_t, uint8_t> GetNextPortAndPriority () const;

  /**
     * \brief Convert rate (PPS) to time per packet.
     *
     * \param pps the rate in packets per second
     * \return the time per packet
     */
  static Time RateToTime (uint64_t pps);

  std::vector<std::vector<FifoQueue>> m_priorityQueues; //!< Priority queues for each port
  uint8_t m_nbPorts{4}; //!< Number of ports
  uint8_t m_nbPriorities{8}; //!< Number of priorities
  Ptr<UniformRandomVariable> m_rng; //!< Random number generator
};

} // namespace ns3

#endif // NS3_P4_RR_PRI_QUEUE_DISC_H
