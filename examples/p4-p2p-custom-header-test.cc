/**
 * 
(p4dev-python-venv) p4@p4:~/workdir/ns3.35$ NS_LOG="CustomP2PNetDevice=debug" ./waf --run "p4-p2p-custom-header-test"
/home/p4/workdir/ns3.35/bindings/python/wscript:321: SyntaxWarning: invalid escape sequence '\d'
  m = re.match( "^castxml version (\d\.\d)(-)?(\w+)?", castxml_version_line)
Waf: Entering directory `/home/p4/workdir/ns3.35/build'
Waf: Leaving directory `/home/p4/workdir/ns3.35/build'
Build commands will be stored in build/compile_commands.json
'build' finished successfully (0.854s)
Ethernet protocolNumber: 0x800
Sending: before adding the ethernet header and custom header
ns3::Ipv4Header (tos 0x0 DSCP Default ECN Not-ECT ttl 64 id 0 protocol 17 offset (bytes) 0 flags [none] length: 1028 10.1.1.1 > 10.1.1.2) ns3::UdpHeader (length: 1008 49153 > 12000) Payload (size=1000) 
UDP protocol, get the port number
Sending: after adding the header
ns3::EthernetHeader ( length/type=0x12, source=00:00:00:00:00:01, destination=ff:ff:ff:ff:ff:ff) ns3::CustomHeader (CustomHeader { }) ns3::Ipv4Header (tos 0x0 DSCP Default ECN Not-ECT ttl 64 id 0 protocol 17 offset (bytes) 0 flags [none] length: 1028 10.1.1.1 > 10.1.1.2) ns3::UdpHeader (length: 1008 49153 > 12000) Payload (size=1000) 
* Ethernet header detecting, packet length: 1050
** Ethernet packet
*** Ethernet header: Source MAC: 00:00:00:00:00:01, Destination MAC: ff:ff:ff:ff:ff:ff, Protocol: 0x12
* Custom header detecting, packet length: 1036
** Custom header detected
*** Custom header content: 
CustomHeader { Field1: 0x800 Field2: 0x1234 Field3: 0x89ABCDEF }
* IPv4 header detecting, packet length: 1028
** IPv4 packet
*** IPv4 header: Source IP: 10.1.1.1, Destination IP: 10.1.1.2, TTL: 64, Protocol: 17
Parser: IPv4 protocol: 17
* UDP header detecting, packet length: 1008
** UDP packet
*** UDP header: Source Port: 49153, Destination Port: 12000
Start reverse parsing
Reverse Parser: UDP Header
Reverse Parser: IPv4 Header
Reverse Parser: Custom P4 Header
ns3::Ipv4Header (tos 0x0 DSCP Default ECN Not-ECT ttl 64 id 0 protocol 17 offset (bytes) 0 flags [none] length: 1028 10.1.1.1 > 10.1.1.2) ns3::UdpHeader (length: 1000 49153 > 12000) Payload (size=1000) 
 * 
 */

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
  ns3::PacketMetadata::Enable ();

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

  // [ethernet] [custom] [ipv4] [udp]

  customHeader.AddField ("Field1", 16); // 16-bit field
  customHeader.AddField ("Field2", 16); // 16-bit field
  customHeader.AddField ("Field3", 32); // 32-bit field

  customHeader.SetField ("Field1", 0x0800); // Protocol : UDP
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

  uint16_t servPort = 12000;

  Ipv4Address serverAddr1 = interfaces.GetAddress (1); // FIXED: Correct interface index
  InetSocketAddress dst1 = InetSocketAddress (serverAddr1, servPort);
  PacketSinkHelper sink1 ("ns3::UdpSocketFactory", dst1);
  ApplicationContainer sinkApp1 = sink1.Install (b);

  sinkApp1.Start (Seconds (sink_start_time));
  sinkApp1.Stop (Seconds (sink_stop_time));

  OnOffHelper onOff1 ("ns3::UdpSocketFactory", dst1);
  onOff1.SetAttribute ("PacketSize", UintegerValue (pktSize));
  onOff1.SetAttribute ("DataRate", StringValue (appDataRate[0]));
  onOff1.SetAttribute ("MaxBytes", UintegerValue (200));
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
