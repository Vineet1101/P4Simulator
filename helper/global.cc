#include "ns3/global.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4GlobalVar");
NS_OBJECT_ENSURE_REGISTERED (P4GlobalVar);

// P4 controller instance
P4Controller P4GlobalVar::g_p4Controller;

// Network function configuration
P4ModuleType P4GlobalVar::g_networkFunc = P4ModuleType::BASIC;
unsigned int P4GlobalVar::g_p4ArchType = P4ARCHV1MODEL;
unsigned int P4GlobalVar::g_populateFlowTableWay = RUNTIME_CLI;
P4ChannelType P4GlobalVar::g_channelType = P4ChannelType::CSMA;

// File paths
std::string P4GlobalVar::g_p4MatchTypePath = "";
std::string P4GlobalVar::g_flowTablePath = "";
std::string P4GlobalVar::g_viewFlowTablePath = "";
std::string P4GlobalVar::g_p4JsonPath = "";

// Bottleneck configuration, packet per second [pps]
uint64_t P4GlobalVar::g_switchBottleNeck = 10000;

// Simulation type and mapping
unsigned int P4GlobalVar::g_nsType = P4Simulator;
std::map<std::string, unsigned int> P4GlobalVar::g_nfStrUintMap;

// PathConfig definition
std::string P4GlobalVar::PathConfig::HomePath = "/home/p4/";
std::string P4GlobalVar::PathConfig::Ns3RootName = "workdir/";
std::string P4GlobalVar::PathConfig::Ns3SrcName = "ns3.35/";
std::string P4GlobalVar::PathConfig::NfDir =
    P4GlobalVar::PathConfig::HomePath + P4GlobalVar::PathConfig::Ns3RootName +
    P4GlobalVar::PathConfig::Ns3SrcName + "contrib/p4sim/examples/test/";
std::string P4GlobalVar::PathConfig::TopoDir =
    P4GlobalVar::PathConfig::HomePath + P4GlobalVar::PathConfig::Ns3RootName +
    P4GlobalVar::PathConfig::Ns3SrcName + "contrib/p4sim/examples/topo/";
std::string P4GlobalVar::PathConfig::FlowTableDir =
    P4GlobalVar::PathConfig::HomePath + P4GlobalVar::PathConfig::Ns3RootName +
    P4GlobalVar::PathConfig::Ns3SrcName + "scratch-p4-file/flowtable/";
std::string P4GlobalVar::PathConfig::ExampleP4SrcDir =
    P4GlobalVar::PathConfig::HomePath + P4GlobalVar::PathConfig::Ns3RootName +
    P4GlobalVar::PathConfig::Ns3SrcName + "contrib/p4sim/examples/p4src/";

TypeId
P4GlobalVar::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P4GlobalVar").SetParent<Object> ().SetGroupName ("P4GlobalVar");
  return tid;
}

void
P4GlobalVar::SetP4MatchTypeJsonPath ()
{
  NS_LOG_INFO ("Setting P4 match type JSON path");

  static const std::map<P4ModuleType, std::string> moduleDirMap = {
      {P4ModuleType::FIREWALL, "firewall"}, {P4ModuleType::SILKROAD, "silkroad"},
      {P4ModuleType::BASIC, "basic"},       {P4ModuleType::SIMPLE_ROUTER, "simple_router"},
      {P4ModuleType::COUNTER, "counter"},   {P4ModuleType::METER, "meter"},
      {P4ModuleType::REGISTER, "register"}, {P4ModuleType::SIMPLE_SWITCH, "simple_switch"}};

  auto it = moduleDirMap.find (g_networkFunc);
  if (it != moduleDirMap.end ())
    {
      std::string moduleDir = it->second;
      g_p4JsonPath = PathConfig::NfDir + moduleDir + "/" + moduleDir + ".json";
      g_p4MatchTypePath = PathConfig::NfDir + moduleDir + "/mtype.txt"; // remove

      // if (g_networkFunc == P4ModuleType::SIMPLE_SWITCH)
      //   {
      //     g_flowTablePath = PathConfig::NfDir + moduleDir + "/flowtable/";
      //   }
      g_flowTablePath = PathConfig::NfDir + moduleDir + "/flowtable.txt";
    }
  else
    {
      NS_LOG_ERROR ("Invalid P4ModuleType");
    }
}

void
P4GlobalVar::InitNfStrUintMap ()
{
  g_nfStrUintMap["BASIC"] = static_cast<unsigned int> (P4ModuleType::BASIC);
  g_nfStrUintMap["FIREWALL"] = static_cast<unsigned int> (P4ModuleType::FIREWALL);
  g_nfStrUintMap["SILKROAD"] = static_cast<unsigned int> (P4ModuleType::SILKROAD);
  g_nfStrUintMap["SIMPLE_ROUTER"] = static_cast<unsigned int> (P4ModuleType::SIMPLE_ROUTER);
  g_nfStrUintMap["COUNTER"] = static_cast<unsigned int> (P4ModuleType::COUNTER);
  g_nfStrUintMap["METER"] = static_cast<unsigned int> (P4ModuleType::METER);
  g_nfStrUintMap["REGISTER"] = static_cast<unsigned int> (P4ModuleType::REGISTER);
  g_nfStrUintMap["SIMPLE_SWITCH"] = static_cast<unsigned int> (P4ModuleType::SIMPLE_SWITCH);
}

uint64_t
getTickCount ()
{
  return std::chrono::duration_cast<std::chrono::milliseconds> (
             std::chrono::steady_clock::now ().time_since_epoch ())
      .count ();
}

} // namespace ns3
