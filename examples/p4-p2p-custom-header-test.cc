#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/packet.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/mac48-address.h"
#include "ns3/simulator.h"
#include "ns3/custom-p2p-net-device.h"
#include "ns3/p4-p2p-channel.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/p4-p2p-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("P4PointToPointCustomHeaderTest");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("P4PointToPointCustomHeaderTest", LOG_LEVEL_INFO);
  uint16_t pktSize = 1000;
  std::string appDataRate[] = {"2.0Mbps", "1.0Mbps"};

  double global_start_time = 1.0;
  double sink_start_time = global_start_time + 1.0;
  double client_start_time = global_start_time + 2.0;
  double client_stop_time = client_start_time + 10;
  double sink_stop_time = client_stop_time + 10;
  double global_stop_time = client_stop_time + 10;

  Ptr<Node> a = CreateObject<Node> ();
  Ptr<Node> b = CreateObject<Node> ();

  P4PointToPointHelper p2phelper;
  p2phelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0.01)));

  NetDeviceContainer devices = p2phelper.Install (a, b);

  // 获取 A 端的 NetDevice，并进行转换
  Ptr<CustomP2PNetDevice> devA = DynamicCast<CustomP2PNetDevice> (devices.Get (0));
  NS_ASSERT (devA != nullptr); // 确保转换成功
  devA->SetWithCustomHeader (true);

  // 获取 A 端的 NetDevice，并进行转换
  Ptr<CustomP2PNetDevice> devB = DynamicCast<CustomP2PNetDevice> (devices.Get (1));
  NS_ASSERT (devB != nullptr); // 确保转换成功
  devB->SetWithCustomHeader (true);

  // Add custom header
  CustomHeader customHeader;
  customHeader.SetLayer (HeaderLayer::LAYER_3);
  customHeader.SetOperator (HeaderLayerOperator::ADD_BEFORE);

  customHeader.AddField ("Field1", 8); // 8-bit field
  customHeader.AddField ("Field2", 16); // 16-bit field
  customHeader.AddField ("Field3", 32); // 32-bit field

  customHeader.SetField ("Field1", 0xAB);
  customHeader.SetField ("Field2", 0x1234);
  customHeader.SetField ("Field3", 0x89ABCDEF);

  devA->SetCustomHeader (customHeader);
  devB->SetCustomHeader (customHeader);

  //   devA->PrintPacketHeaders ();
  InternetStackHelper internet;
  internet.Install (a);
  internet.Install (b);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");

  // FIXED: Assign IPs properly
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  uint16_t servPort = 9093;

  Ipv4Address serverAddr1 = interfaces.GetAddress (1); // FIXED: Correct interface index
  InetSocketAddress dst1 = InetSocketAddress (serverAddr1, servPort);
  PacketSinkHelper sink1 ("ns3::UdpSocketFactory", dst1);
  ApplicationContainer sinkApp1 = sink1.Install (b);

  sinkApp1.Start (Seconds (sink_start_time));
  sinkApp1.Stop (Seconds (sink_stop_time));

  OnOffHelper onOff1 ("ns3::UdpSocketFactory", dst1);
  onOff1.SetAttribute ("PacketSize", UintegerValue (pktSize));
  onOff1.SetAttribute ("DataRate", StringValue (appDataRate[0]));
  onOff1.SetAttribute ("MaxBytes", UintegerValue (1200));
  onOff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  ApplicationContainer app1 = onOff1.Install (a);
  app1.Start (Seconds (client_start_time));
  app1.Stop (Seconds (client_stop_time));

  p2phelper.EnablePcapAll ("p4-p2p-custom-header-test");
  Simulator::Stop (Seconds (global_stop_time));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
