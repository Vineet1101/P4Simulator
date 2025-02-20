#ifndef GLOBAL_H
#define GLOBAL_H

#include "ns3/object.h"
#include "ns3/p4-controller.h"
#include <map>
#include <string>
#include <chrono> // For cross-platform time acquisition

namespace ns3 {

// Method to configure the flow table in P4 switch
#define LOCAL_CALL 0 // Local call to configure the flow table
#define RUNTIME_CLI 1 // Use runtime CLI to configure the flow table
#define NS3PIFOTM 2 // Use thrift port to configure the flow table

// Network function type
#define NS3 1 // Normal switch
#define P4Simulator 0 // P4 switch

#define P4ARCHV1MODEL 0 // P4 architecture v1model
#define P4ARCHPSA 1 // P4 architecture PSA

enum class P4ModuleType {
  BASIC,
  FIREWALL,
  SILKROAD,
  SIMPLE_ROUTER,
  COUNTER,
  METER,
  REGISTER,
  SIMPLE_SWITCH
};

enum class P4ChannelType { CSMA, P2P };

uint64_t getTickCount (); // Get current time (ms)

class P4Controller;

class P4GlobalVar : public Object
{
public:
  static TypeId GetTypeId (void);

  // Controller
  static P4Controller g_p4Controller;

  // Network function configuration
  static P4ModuleType g_networkFunc;
  static unsigned int g_p4ArchType;
  static unsigned int g_populateFlowTableWay;
  static P4ChannelType g_channelType;

  // Path configuration
  struct PathConfig
  {
    static std::string HomePath;
    static std::string Ns3RootName;
    static std::string Ns3SrcName;
    static std::string NfDir;
    static std::string TopoDir;
    static std::string FlowTableDir;
    static std::string ExampleP4SrcDir;
  };

  // File paths
  static std::string g_p4MatchTypePath;
  static std::string g_flowTablePath;
  static std::string g_viewFlowTablePath;
  static std::string g_p4JsonPath;

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
      {{"proto_id", 16}, {"dst_id", 16}}};

  // Bottleneck configuration
  static uint64_t g_switchBottleNeck;

  // Simulation type and mapping
  static unsigned int g_nsType;
  static std::map<std::string, unsigned int> g_nfStrUintMap;

  // Utility functions
  static void SetP4MatchTypeJsonPath ();
  static void InitNfStrUintMap ();

private:
  P4GlobalVar ();
  ~P4GlobalVar ();
  P4GlobalVar (const P4GlobalVar &);
  P4GlobalVar &operator= (const P4GlobalVar &);
};

} // namespace ns3

#endif /* GLOBAL_H */
