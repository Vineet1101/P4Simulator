// 测试p2p信道，单点收发

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  // 创建节点
  NodeContainer hosts;
  hosts.Create (2); // Host1 和 Host2

  // 创建 P2P 链路
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices = p2p.Install (hosts);

  // 安装协议栈
  InternetStackHelper stack;
  stack.Install (hosts);

  // 分配 IP 地址
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  // 设置 OnOffApplication
  uint16_t port = 9; // 服务端的端口号
  Address serverAddress = InetSocketAddress (interfaces.GetAddress (1), port); // Host2 的 IP 和端口

  OnOffHelper onOff ("ns3::UdpSocketFactory", serverAddress);
  onOff.SetConstantRate (DataRate ("1Mbps"), 1024); // 数据发送速率：1Mbps，每个数据包大小：1024字节
  onOff.SetAttribute ("StartTime", TimeValue (Seconds (2.0))); // 开始时间
  onOff.SetAttribute ("StopTime", TimeValue (Seconds (10.0))); // 结束时间

  ApplicationContainer clientApps = onOff.Install (hosts.Get (0)); // 在 Host1 上安装应用

  // 设置接收端应用
  PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory",
                                     InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer serverApps = packetSinkHelper.Install (hosts.Get (1)); // 在 Host2 上安装应用
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // 开启 PCAP 捕获
  p2p.EnablePcapAll ("p2p-onoff-example");

  // 启动模拟器
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
