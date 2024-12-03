#include "ns3/priority-port-tag.h"
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

PriorityPortTag::PriorityPortTag (uint32_t priority, uint32_t port)
  : m_priority (priority), m_port (port)
{
}

void
PriorityPortTag::SetPriority (uint32_t priority)
{
  m_priority = priority;
}

uint32_t
PriorityPortTag::GetPriority (void) const
{
  return m_priority;
}

void
PriorityPortTag::SetPort (uint32_t port)
{
  m_port = port;
}

uint32_t
PriorityPortTag::GetPort (void) const
{
  return m_port;
}

uint32_t
PriorityPortTag::GetSerializedSize (void) const
{
  return sizeof(uint32_t) + sizeof(uint32_t);
}

void
PriorityPortTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_priority);
  i.WriteU32 (m_port);
}

void
PriorityPortTag::Deserialize (TagBuffer i)
{
  m_priority = i.ReadU32 ();
  m_port = i.ReadU32 ();
}

void
PriorityPortTag::Print (std::ostream &os) const
{
  os << "Priority=" << m_priority
     << ", Port=" << m_port;
}

} // namespace ns3
