// PriorityPortTag.cc
#include "priority-port-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PriorityPortTag");
NS_OBJECT_ENSURE_REGISTERED (PriorityPortTag);

TypeId
PriorityPortTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PriorityPortTag")
    .SetParent<Tag> ()
    .AddConstructor<PriorityPortTag> ();
  return tid;
}

TypeId
PriorityPortTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

PriorityPortTag::PriorityPortTag ()
  : m_priority (0), m_port (0)
{
}

PriorityPortTag::PriorityPortTag (uint8_t priority, uint16_t port)
  : m_priority (priority), m_port (port)
{
}

void
PriorityPortTag::SetPriority (uint8_t priority)
{
  m_priority = priority;
}

uint8_t
PriorityPortTag::GetPriority (void) const
{
  return m_priority;
}

void
PriorityPortTag::SetPort (uint16_t port)
{
  m_port = port;
}

uint16_t
PriorityPortTag::GetPort (void) const
{
  return m_port;
}

uint32_t
PriorityPortTag::GetSerializedSize (void) const
{
  return sizeof(uint8_t) + sizeof(uint16_t);
}

void
PriorityPortTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_priority);
  i.WriteU16 (m_port);
}

void
PriorityPortTag::Deserialize (TagBuffer i)
{
  m_priority = i.ReadU8 ();
  m_port = i.ReadU16 ();
}

void
PriorityPortTag::Print (std::ostream &os) const
{
  os << "Priority=" << static_cast<uint32_t>(m_priority)
     << ", Port=" << m_port;
}

} // namespace ns3
