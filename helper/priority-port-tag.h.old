#ifndef PRIORITY_PORT_TAG_H
#define PRIORITY_PORT_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {

class PriorityPortTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const override;

  PriorityPortTag ();
  PriorityPortTag (uint32_t priority, uint32_t port);

  void SetPriority (uint32_t priority);
  uint32_t GetPriority (void) const;

  void SetPort (uint32_t port);
  uint32_t GetPort (void) const;

  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;

  uint32_t GetSerializedSize (void) const override;
  void Print (std::ostream &os) const override;

private:
  uint32_t m_priority;
  uint32_t m_port;
};

} // namespace ns3

#endif // PRIORITY_PORT_TAG_H
