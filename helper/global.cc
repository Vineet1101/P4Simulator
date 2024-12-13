/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/global.h"
#include "ns3/log.h"

#if defined(WIN32)
#include <windows.h>
#elif defined(OS_VXWORKS)
#include <sysLib.h>
#include <vxWorks.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#endif

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4GlobalVar");
NS_OBJECT_ENSURE_REGISTERED (P4GlobalVar);

// P4 controller instance
P4Controller P4GlobalVar::g_p4Controller;

/*******************************
 * P4 switch configuration info
 *******************************/

// Global configuration variables for P4 switch
unsigned int P4GlobalVar::g_networkFunc = SIMPLESWITCH;
std::string P4GlobalVar::g_flowTablePath = "";
std::string P4GlobalVar::g_viewFlowTablePath = "";
std::string P4GlobalVar::g_p4MatchTypePath = "";
unsigned int P4GlobalVar::g_populateFlowTableWay = RUNTIME_CLI; // LOCAL_CALL or RUNTIME_CLI
std::string P4GlobalVar::g_p4JsonPath = "";
int P4GlobalVar::g_switchBottleNeck = 10000;

// Directories for P4 simulation resources
std::string P4GlobalVar::g_homePath = "/home/p4/";
std::string P4GlobalVar::g_ns3RootName = "/";
std::string P4GlobalVar::g_ns3SrcName = "ns-3-dev-git/";

std::string P4GlobalVar::g_nfDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
                                   P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/p4src/";
std::string P4GlobalVar::g_topoDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
                                     P4GlobalVar::g_ns3SrcName + "src/p4simulator/examples/topo/";
std::string P4GlobalVar::g_flowTableDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
                                          P4GlobalVar::g_ns3SrcName + "scratch-p4-file/flowtable/";
std::string P4GlobalVar::g_exampleP4SrcDir = P4GlobalVar::g_homePath + P4GlobalVar::g_ns3RootName +
                                             P4GlobalVar::g_ns3SrcName +
                                             "src/p4simulator/examples/p4src/";

// Simulation type
unsigned int P4GlobalVar::g_nsType = P4Simulator;
std::map<std::string, unsigned int> P4GlobalVar::g_nfStrUintMap;

/*******************************
 * Get current system time in milliseconds
 *******************************/
uint64_t
getTickCount ()
{
  uint64_t currentTime = 0;

#if __cplusplus >= 201103L // C++11 or later
  // Use std::chrono for cross-platform time acquisition
  auto now = std::chrono::steady_clock::now ();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (now.time_since_epoch ());
  currentTime = duration.count ();
#elif defined(WIN32)
  // Windows platform
  currentTime = GetTickCount ();
#elif defined(OS_VXWORKS)
  // VXWorks platform
  ULONGA timeSecond = tickGet () / sysClkRateGet ();
  ULONGA timeMilsec = tickGet () % sysClkRateGet () * 1000 / sysClkRateGet ();
  currentTime = static_cast<uint64_t> (timeSecond) * 1000 + timeMilsec;
#else
  // Unix/Linux platform
  struct timeval current;
  gettimeofday (&current, NULL);
  currentTime = static_cast<uint64_t> (current.tv_sec) * 1000 + current.tv_usec / 1000;
#endif

  return currentTime;
}

/*******************************
 * Set P4 Match Type and JSON paths
 *******************************/
void
P4GlobalVar::SetP4MatchTypeJsonPath ()
{
  switch (P4GlobalVar::g_networkFunc)
    {
    case FIREWALL:
      g_p4JsonPath = g_nfDir + "firewall/firewall.json";
      g_p4MatchTypePath = g_nfDir + "firewall/mtype.txt";
      break;
    case SILKROAD:
      g_p4JsonPath = g_nfDir + "silkroad/silkroad.json";
      g_p4MatchTypePath = g_nfDir + "silkroad/mtype.txt";
      break;
    case ROUTER:
      g_p4JsonPath = g_nfDir + "router/router.json";
      g_p4MatchTypePath = g_nfDir + "router/mtype.txt";
      break;
    case SIMPLE_ROUTER:
      g_p4JsonPath = g_nfDir + "simple_router/simple_router.json";
      g_p4MatchTypePath = g_nfDir + "simple_router/mtype.txt";
      break;
    case COUNTER:
      g_p4JsonPath = g_nfDir + "counter/counter.json";
      g_p4MatchTypePath = g_nfDir + "counter/mtype.txt";
      break;
    case METER:
      g_p4JsonPath = g_nfDir + "meter/meter.json";
      g_p4MatchTypePath = g_nfDir + "meter/mtype.txt";
      break;
    case REGISTER:
      g_p4JsonPath = g_nfDir + "register/register.json";
      g_p4MatchTypePath = g_nfDir + "register/mtype.txt";
      break;
    case SIMPLESWITCH:
      g_p4JsonPath = g_nfDir + "simple_switch/simple_switch.json";
      g_flowTableDir = g_nfDir + "simple_switch/flowtable/";
      break;
    case PRIORITYQUEUE:
      g_p4JsonPath = g_nfDir + "priority_queuing/priority_queuing.json";
      g_flowTableDir = g_nfDir + "priority_queuing/flowtable/";
      break;
    case SIMPLECODEL:
      g_p4JsonPath = g_nfDir + "simple_codel/simple_codel.json";
      g_flowTableDir = g_nfDir + "simple_codel/flowtable/";
      break;
    case CODELPP:
      g_p4JsonPath = g_nfDir + "codelpp/codel1.json";
      g_flowTableDir = g_nfDir + "codelpp/flowtable/";
      break;
    default:
      std::cerr << "NETWORK_FUNCTION_NO_EXIST!!!" << std::endl;
      break;
    }
}

/*******************************
 * Initialize network function string-to-ID map
 *******************************/
void
P4GlobalVar::InitNfStrUintMap ()
{
  g_nfStrUintMap["ROUTER"] = ROUTER;
  g_nfStrUintMap["SIMPLE_ROUTER"] = SIMPLE_ROUTER;
  g_nfStrUintMap["FIREWALL"] = FIREWALL;
  g_nfStrUintMap["SILKROAD"] = SILKROAD;
  g_nfStrUintMap["COUNTER"] = COUNTER;
  g_nfStrUintMap["METER"] = METER;
  g_nfStrUintMap["REGISTER"] = REGISTER;
  g_nfStrUintMap["SIMPLESWITCH"] = SIMPLESWITCH;
  g_nfStrUintMap["PRIORITYQUEUE"] = PRIORITYQUEUE;
  g_nfStrUintMap["SIMPLECODEL"] = SIMPLECODEL;
  g_nfStrUintMap["CODELPP"] = CODELPP;
}

/*******************************
 * NS-3 Object TypeId system
 *******************************/
TypeId
P4GlobalVar::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4GlobalVar").SetParent<Object> ().SetGroupName ("P4GlobalVar");
  return tid;
}

// Constructor
P4GlobalVar::P4GlobalVar ()
{
  NS_LOG_FUNCTION (this);
}

// Destructor
P4GlobalVar::~P4GlobalVar ()
{
  NS_LOG_FUNCTION (this);
}

} // namespace ns3
