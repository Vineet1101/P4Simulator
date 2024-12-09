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

#ifndef NS3_P4_RR_PRI_QUEUE_DISC_H
#define NS3_P4_RR_PRI_QUEUE_DISC_H

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

  //   /**
  //      * \brief Get the queue length for a given priority and port.
  //      *
  //      * \param port the port index
  //      * \param priority the priority index
  //      * \return the queue length
  //      */
  //   uint32_t GetQueueSize (uint8_t port, uint8_t priority) const;

  //   /**
  //      * \brief Get the capacity of the queue for a given priority and port.
  //      *
  //      * \param port the port index
  //      * \param priority the priority index
  //      * \return the queue capacity (number of packets)
  //      */
  //   uint32_t GetQueueCapacity (uint8_t port, uint8_t priority) const;

  //   /**
  //      * \brief Get the rate of the queue for a given priority and port.
  //      *
  //      * \param port the port index
  //      * \param priority the priority index
  //      * \return the rate in packets per second (PPS)
  //      */
  //   uint64_t GetQueueRate (uint8_t port, uint8_t priority) const;

  //   /**
  //      * \brief Set the capacity of the queue for a given priority and port.
  //      *
  //      * \param port the port index
  //      * \param priority the priority index
  //      * \param capacity the capacity (number of packets)
  //      */
  //   void SetQueueCapacity (uint8_t port, uint8_t priority, uint32_t capacity);

  //   /**
  //      * \brief Set the rate of the queue for a given priority and port.
  //      *
  //      * \param port the port index
  //      * \param priority the priority index
  //      * \param ratePps the rate in packets per second (PPS)
  //      */
  //   void SetQueueRate (uint8_t port, uint8_t priority, uint64_t ratePps);

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
  //   struct FifoQueue
  //   {
  //     std::queue<Ptr<P4QueueItem>> queue; //!< FIFO queue of packets
  //     uint32_t capacity{1000}; //!< Maximum capacity in packets
  //     uint64_t ratePps{1000}; //!< Rate in packets per second
  //     Time delayTime{MilliSeconds (1)}; //!< Delay time per packet
  //   };

  //   /**
  //      * \brief Calculate the next eligible send time for a packet in the queue.
  //      *
  //      * \param queue the priority queue
  //      * \return the calculated next send time
  //      */
  //   Time CalculateNextSendTime (const FifoQueue &queue) const;

  /**
     * \brief Find the next port with a packet ready to dequeue.
     *
     * \return a pair containing the port index
     */
  uint32_t GetNextPort () const;

  bool is_peek_marked{false}; //!< Flag to indicate if the peeked packet is marked
  uint32_t marked_port{0}; //!< Port of the marked packet

  uint8_t m_nbPorts{4}; //!< Number of ports
  uint8_t m_nbPriorities{8}; //!< Number of priorities
  Ptr<UniformRandomVariable> m_rng; //!< Random number generator
};

} // namespace ns3

#endif // NS3_P4_RR_PRI_QUEUE_DISC_H
