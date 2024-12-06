#include "ns3/p4-queue-item.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4QueueItem");

// TypeId
// P4QueueItem::GetTypeId()
// {
//     static TypeId tid =
//         TypeId("ns3::P4QueueItem").SetParent<QueueDiscItem>().AddConstructor<P4QueueItem>();
//     return tid;
// }

P4QueueItem::P4QueueItem (Ptr<Packet> p, const Address &addr, uint16_t protocol)
    : QueueDiscItem (p, addr, protocol), m_sendTime (Seconds (0))
{
  NS_LOG_FUNCTION (this << p << addr << protocol);
}

P4QueueItem::~P4QueueItem ()
{
  NS_LOG_FUNCTION (this);
}

void
P4QueueItem::SetSendTime (Time sendTime)
{
  NS_LOG_FUNCTION (this << sendTime);
  m_sendTime = sendTime;
}

Time
P4QueueItem::GetSendTime () const
{
  NS_LOG_FUNCTION (this);
  return m_sendTime;
}

bool
P4QueueItem::IsReadyToDequeue (Time currentTime) const
{
  NS_LOG_FUNCTION (this << currentTime);
  return currentTime >= m_sendTime;
}

void
P4QueueItem::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this);
  os << "P4QueueItem: " << "SendTime=" << m_sendTime.GetSeconds () << "s, "
     << "PacketSize=" << GetPacket ()->GetSize () << " bytes";
}

} // namespace ns3
