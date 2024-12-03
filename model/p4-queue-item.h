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
  
  // /**
  //    * \brief Get the type ID.
  //    * \return the object TypeId
  //    */
  //   static TypeId GetTypeId();
  

  /**
   * \brief Constructor
   * \param p The packet
   * \param addr The address of the packet
   * \param protocol The protocol of the packet
   */
  P4QueueItem (Ptr<Packet> p, const Address &addr, uint16_t protocol);

  virtual ~P4QueueItem () override;

  // Delete default constructor, copy constructor and assignment operator to avoid misuse
  P4QueueItem () = delete;
  P4QueueItem (const P4QueueItem &) = delete;
  P4QueueItem &operator = (const P4QueueItem &) = delete;

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

  void AddHeader() override
  {
    // @TODO deal with header, keep header and payload separate to allow manipulating the header
    return;
  }

  /**
   * \brief Marks the packet as a substitute for dropping it, such as for Explicit Congestion
   * Notification
   *
   * \return true if the packet is marked by this method or is already marked, false otherwise
   */
  bool Mark() override
  {
    // 
    return false;
  }

private:
  Time m_sendTime; //!< Expected send time of the packet
};

} // namespace ns3

#endif // P4_QUEUE_ITEM_H
