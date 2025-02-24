#include "ns3/global.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4GlobalVar");
NS_OBJECT_ENSURE_REGISTERED (P4GlobalVar);

TypeId
P4GlobalVar::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4GlobalVar").SetParent<Object> ().SetGroupName ("P4GlobalVar");
  return tid;
}

} // namespace ns3
