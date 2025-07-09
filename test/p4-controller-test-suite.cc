/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/core-module.h"
#include "ns3/csma-helper.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/p4-controller.h"
#include "ns3/p4-core-v1model.h"
#include "ns3/p4-helper.h"
#include "ns3/p4-topology-reader-helper.h"
#include "ns3/test.h"

using namespace ns3;

class P4ControllerCheckFlowEntryTestCase : public TestCase {
public:
  P4ControllerCheckFlowEntryTestCase();

  virtual ~P4ControllerCheckFlowEntryTestCase();

private:
  void DoRun() override;
};
P4ControllerCheckFlowEntryTestCase::P4ControllerCheckFlowEntryTestCase()
    : TestCase("P4 controller add flow entry test case (does nothing)") {}

P4ControllerCheckFlowEntryTestCase::~P4ControllerCheckFlowEntryTestCase() {}

void P4ControllerCheckFlowEntryTestCase::DoRun() {
  // std::string appDataRate = "3Mbps"; // Default application data rate
  // std::string ns3_link_rate = "1000Mbps";

  P4TopologyReaderHelper p4TopoHelper;
  p4TopoHelper.SetFileName("/home/p4/workdir/ns3.39/contrib/p4sim/examples/"
                           "p4src/simple_v1model/topo.txt");
  p4TopoHelper.SetFileType("CsmaTopo");

  // Get the topology reader, and read the file, load in the m_linksList.
  Ptr<P4TopologyReader> topoReader = p4TopoHelper.GetTopologyReader();

  NS_TEST_ASSERT_MSG_NE(topoReader->LinksSize(), 0,
                        "Verify that we reading the topology file correctly");

  // get switch and host node
  NodeContainer terminals = topoReader->GetHostNodeContainer();
  NodeContainer switchNode = topoReader->GetSwitchNodeContainer();

  // Install the network stack on the nodes
  InternetStackHelper internet;
  internet.Install(terminals);
  internet.Install(switchNode);
  const unsigned int hostNum = terminals.GetN();

  // Assign IP addresses to the hosts
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  std::vector<Ipv4InterfaceContainer> terminalInterfaces(hostNum);

  for (unsigned int i = 0; i < hostNum; i++) {
    terminalInterfaces[i] = ipv4.Assign(terminals.Get(i)->GetDevice(0));
  }

  // Set up the P4 switch helper
  P4Helper p4Helper;
  p4Helper.SetDeviceAttribute(
      "JsonPath", StringValue("/home/p4/workdir/ns3.39/contrib/p4sim/test/"
                              "p4src/simple_v1model/simple_v1model.json"));
  p4Helper.SetDeviceAttribute(
      "FlowTablePath", StringValue("/home/p4/workdir/ns3.39/contrib/p4sim/test/"
                                   "p4src/simple_v1model/flowtable_0.txt"));
  p4Helper.SetDeviceAttribute("ChannelType", UintegerValue(0));
  p4Helper.SetDeviceAttribute("P4SwitchArch", UintegerValue(0)); // v1model

  // Setting up the controller
  P4Controller controller;

  for (uint32_t i = 0; i < switchNode.GetN(); ++i) {
    NetDeviceContainer devs;
    for (uint32_t j = 0; j < switchNode.Get(i)->GetNDevices(); ++j) {
      Ptr<NetDevice> dev = switchNode.Get(i)->GetDevice(j);
      devs.Add(dev);
    }

    NetDeviceContainer p4Devices = p4Helper.Install(switchNode.Get(i), devs);

    for (uint32_t k = 0; k < p4Devices.GetN(); ++k) {
      Ptr<P4SwitchNetDevice> p4dev =
          DynamicCast<P4SwitchNetDevice>(p4Devices.Get(k));
      if (p4dev) {
        controller.RegisterSwitch(p4dev);
      }
    }
  }

  // In running time, we can check the table entries from the P4 switches
  Simulator::Schedule(Seconds(2.0), [this, &controller]() {
    NS_TEST_ASSERT_MSG_EQ(controller.GetN(), 1,
                          "There should be only one p4switch in there");

    NS_TEST_ASSERT_MSG_EQ(
        controller.GetTableEntryCount(0, "MyIngress.ipv4_nhop"), 2,
        "Verify that we could get the right count of table entries in the "
        "controller");

    NS_TEST_ASSERT_MSG_EQ(
        controller.GetTableEntryCount(0, "MyIngress.arp_simple"), 2,
        "Verify that we could get the right count of table entries in the "
        "controller");

    uint32_t index = 0;
    std::string table = "MyIngress.ipv4_nhop";
    std::string originalAction = "MyIngress.drop";
    std::string newAction = "MyIngress.ipv4_forward";

    std::vector<bm::MatchKeyParam> match = {bm::MatchKeyParam(
        bm::MatchKeyParam::Type::EXACT, std::string("\x0a\x01\x01\x05", 4))};

    bm::ActionData originalActionData;

    bm::entry_handle_t handle;
    controller.AddFlowEntry(index, table, match, originalAction,
                            std::move(originalActionData), handle, -1);
    NS_TEST_ASSERT_MSG_EQ(controller.GetTableEntryCount(index, table), 3,
                          "Flow entry should have been added");
    controller.PrintFlowEntries(0, table);
    bm::ActionData newActionData;

    bm::Data dstMacData;
    dstMacData.set(std::string("\x00\x00\x00\x00\x00\x09", 6));
    newActionData.push_back_action_data(dstMacData);

    bm::Data portData;
    portData.set(std::string("\x00\x02", 2));
    newActionData.push_back_action_data(portData);

    controller.ModifyFlowEntry(index, table, handle, newAction,
                               std::move(newActionData));
    controller.PrintFlowEntries(0, table);

    controller.DeleteFlowEntry(0, table, handle);
    NS_TEST_ASSERT_MSG_EQ(controller.GetTableEntryCount(index, table), 2,

                          "Flow entry should have been added");
    // TODO: Add support_timeout=true in p4 programs and compile them again
    uint32_t ttlMs = 3000;
    controller.SetEntryTtl(index, table, handle, ttlMs);

    NS_TEST_EXPECT_MSG_EQ(controller.GetTableEntryCount(index, table), 2,
                          "Flow entry with TTL should exist in the table");
  });
  Simulator::Stop(Seconds(2.0));
  Simulator::Run();
  Simulator::Destroy();
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined
//
class P4ControllerTestSuite : public TestSuite {
public:
  P4ControllerTestSuite() : TestSuite("p4-controller", UNIT) {
    // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
    AddTestCase(new P4ControllerCheckFlowEntryTestCase, TestCase::QUICK);
  };
} g_P4ControllerTestSuite;
