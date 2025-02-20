#ifndef GLOBAL_H
#define GLOBAL_H

#include "ns3/object.h"
#include <map>
#include <string>
#include <array>
#include <utility>
#include <cstdint>
#include <chrono> // For cross-platform time acquisition

namespace ns3 {

#define P4ARCHV1MODEL 0 // P4 architecture v1model
#define P4ARCHPSA 1 // P4 architecture PSA

enum class P4ChannelType { CSMA, P2P };

uint64_t getTickCount (); // Get current time (ms)

class P4GlobalVar : public Object
{
public:
  static TypeId GetTypeId (void);

  // Network function configuration
  // static P4ModuleType g_networkFunc;
  static unsigned int g_p4ArchType;
  static P4ChannelType g_channelType;

  // Protocol number used to identify the custom P4 header in nested packet parsing.
  // The previous header's protocol field indicates that the next is a custom header
  static constexpr uint64_t g_p4Protocol = 0x12;

  // Port range used to determine whether a packet should include the custom header.
  // In NS-3, the same port may handle different types of packets.
  // If the destination port in the UDP/TCP header falls within [g_portRangeMin, g_portRangeMax],
  // the packet will be assigned a custom header.
  static constexpr uint64_t g_portRangeMin = 10000;
  static constexpr uint64_t g_portRangeMax = 20000;

  // Defines the structure and field lengths of the custom header.
  // Users should modify this based on simulation requirements.
  static constexpr std::array<std::pair<const char *, uint32_t>, 2> g_templateHeaderFields = {
      std::make_pair ("field1", 32), std::make_pair ("field2", 64)};

private:
  P4GlobalVar ();
  ~P4GlobalVar ();
  P4GlobalVar (const P4GlobalVar &);
  P4GlobalVar &operator= (const P4GlobalVar &);
};

} // namespace ns3

#endif /* GLOBAL_H */
