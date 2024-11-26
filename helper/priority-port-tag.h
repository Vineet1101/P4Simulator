// PriorityPortTag.h
#ifndef PRIORITY_PORT_TAG_H
#define PRIORITY_PORT_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"

namespace ns3 {

class PriorityPortTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const override;

  PriorityPortTag ();
  PriorityPortTag (uint8_t priority, uint16_t port);

  void SetPriority (uint8_t priority);
  uint8_t GetPriority (void) const;

  void SetPort (uint16_t port);
  uint16_t GetPort (void) const;

  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;

  uint32_t GetSerializedSize (void) const override;
  void Print (std::ostream &os) const override;

private:
  uint8_t m_priority;
  uint16_t m_port;
};

} // namespace ns3

#endif // PRIORITY_PORT_TAG_H
