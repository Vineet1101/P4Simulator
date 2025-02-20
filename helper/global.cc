#include "ns3/global.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4GlobalVar");
NS_OBJECT_ENSURE_REGISTERED (P4GlobalVar);

unsigned int P4GlobalVar::g_p4ArchType = P4ARCHV1MODEL;
P4ChannelType P4GlobalVar::g_channelType = P4ChannelType::CSMA;

TypeId
P4GlobalVar::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4GlobalVar").SetParent<Object> ().SetGroupName ("P4GlobalVar");
  return tid;
}

uint64_t
getTickCount ()
{
  return std::chrono::duration_cast<std::chrono::milliseconds> (
             std::chrono::steady_clock::now ().time_since_epoch ())
      .count ();
}

} // namespace ns3
