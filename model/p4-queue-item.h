#ifndef P4_QUEUE_ITEM_H
#define P4_QUEUE_ITEM_H

#include "ns3/queue-item.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"

namespace ns3 {

class P4QueueItem : public QueueDiscItem
{
public:
  /**
   * \brief Constructor
   * \param p The packet
   * \param addr The address of the packet
   * \param protocol The protocol of the packet
   */
  P4QueueItem (Ptr<Packet> p, const Address &addr, uint16_t protocol);

  /**
   * \brief Destructor
   */
  virtual ~P4QueueItem ();

  /**
   * \brief Set the expected send time
   * \param sendTime The expected send time
   */
  void SetSendTime (Time sendTime);

  /**
   * \brief Get the expected send time
   * \return The expected send time
   */
  Time GetSendTime () const;

  /**
   * \brief Check if the item is ready for dequeue based on the current time
   * \return True if the item is ready, false otherwise
   */
  bool IsReadyToDequeue (Time currentTime) const;

  /**
   * \brief Print the queue item details
   * \param os The output stream
   */
  void Print (std::ostream &os) const override;

private:
  Time m_sendTime; //!< Expected send time of the packet
};

} // namespace ns3

#endif // P4_QUEUE_ITEM_H
