#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RandomUdpFlowTest");

struct SwitchInfoTracing
{
    bool first_tx = true;
    bool first_rx = true;
    int counter_sender_10 = 2;
    int counter_receiver_10 = 2;
    uint64_t totalTxBytes = 0;
    uint64_t totalTxBytes_lasttime = 0;
    uint64_t totalRxBytes = 0;
    uint64_t totalRxBytes_lasttime = 0;
    uint64_t totalPackets = 0;
    uint64_t totalPackets_lasttime = 0;
    double first_packet_send_time_tx = 0.0;
    double last_packet_send_time_tx = 0.0;
    double first_packet_received_time_rx = 0.0;
    double last_packet_received_time_rx = 0.0;
};

SwitchInfoTracing host_0;

void
TxCallback_host_0(Ptr<const Packet> packet)
{
    host_0.totalPackets++;
    if (host_0.first_tx)
    {
        // here we just simple jump the first 10 pkts (include some of ARP packets)
        host_0.first_packet_send_time_tx = Simulator::Now().GetSeconds();
        host_0.counter_receiver_10--;
        if (host_0.counter_receiver_10 == 0)
        {
            host_0.first_tx = false;
        }
    }
    else
    {
        host_0.totalTxBytes += packet->GetSize();
        host_0.last_packet_send_time_tx = Simulator::Now().GetSeconds();
    }
    NS_LOG_DEBUG("Packet transmitted. Size: " << packet->GetSize()
                                              << " bytes, Total packets: " << host_0.totalPackets
                                              << ", Total bytes: " << host_0.totalTxBytes);
}

// 统计吞吐量并打印
void
CalculateThroughput()
{
    double currentTime = Simulator::Now().GetSeconds();

    double throughput_host_0 =
        ((host_0.totalTxBytes - host_0.totalTxBytes_lasttime) * 8.0) / 1e6; // Mbps

    host_0.totalTxBytes_lasttime = host_0.totalTxBytes;

    // 打印吞吐量信息
    NS_LOG_INFO("Time: " << currentTime << "s | Throughput (Mbps) - "
                         << "Host0(Tx): " << throughput_host_0);

    // 追加写入文件
    std::ofstream outFile("p2p_test_throughput_log.txt", std::ios::app);
    if (outFile.is_open())
    {
        outFile << currentTime << " " << throughput_host_0 << "\n";
        outFile.close();
    }
    else
    {
        NS_LOG_ERROR("Unable to open file for writing.");
    }

    // 每秒再次调用自身
    Simulator::Schedule(Seconds(1.0), &CalculateThroughput);
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("RandomUdpFlowTest", LOG_LEVEL_INFO);

    uint16_t servPortStart = 9000;
    uint16_t servPortEnd = 10000;

    double global_start_time = 1.0;
    double sink_start_time = global_start_time + 1.0;
    double client_start_time = sink_start_time + 1.0;
    double client_stop_time = client_start_time + 60;
    double sink_stop_time = client_stop_time + 5;
    double global_stop_time = sink_stop_time + 5;
    uint32_t pktSize = 1000;
    std::string appDataRate = "10Mbps";

    CommandLine cmd;
    cmd.Parse(argc, argv);

    // **创建两个节点**
    NodeContainer terminals;
    terminals.Create(2);

    // **创建 P2P 设备**
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ns"));

    NetDeviceContainer devices = p2p.Install(terminals);

    // **安装 Internet 协议**
    InternetStackHelper internet;
    internet.Install(terminals);

    // **分配 IP 地址**
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    unsigned int serverI = 1;
    unsigned int clientI = 0;

    Ptr<Node> serverNode = terminals.Get(serverI);
    Ptr<Ipv4> ipv4_adder = serverNode->GetObject<Ipv4>();
    Ipv4Address serverAddr = ipv4_adder->GetAddress(1, 0).GetLocal();

    // **在 Server 端创建 PacketSink，仅监听 servPortStart 端口**
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), servPortStart));
    ApplicationContainer sinkApp = sink.Install(serverNode);
    sinkApp.Start(Seconds(sink_start_time));
    sinkApp.Stop(Seconds(sink_stop_time));

    // **在 Client 端创建多个 UDP 数据流，每个流使用不同的源端口**
    Ptr<Node> clientNode = terminals.Get(clientI);

    for (uint16_t port = servPortStart; port < servPortEnd; port++)
    {
        InetSocketAddress dst(serverAddr, servPortStart); // 目标端口始终为 servPortStart
        dst.SetTos(0xb8);                                 // 设置 DiffServ（可选）

        // 配置客户端 (OnOffApplication)
        OnOffHelper onOff("ns3::UdpSocketFactory", dst);
        onOff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onOff.SetAttribute("DataRate", StringValue(appDataRate));

        // 创建随机开启 (OnTime) 和关闭 (OffTime) 时间
        Ptr<ExponentialRandomVariable> onTime = CreateObject<ExponentialRandomVariable>();
        Ptr<ExponentialRandomVariable> offTime = CreateObject<ExponentialRandomVariable>();

        onTime->SetAttribute("Mean", DoubleValue(2.0));
        offTime->SetAttribute("Mean", DoubleValue(1.0));

        // 使用不同的随机种子，确保流量随机
        onTime->SetAttribute("Stream", IntegerValue(port));
        offTime->SetAttribute("Stream", IntegerValue(port + 1000));

        onOff.SetAttribute("OnTime", PointerValue(onTime));
        onOff.SetAttribute("OffTime", PointerValue(offTime));

        // **指定客户端使用不同的源端口**
        onOff.SetAttribute("Protocol", TypeIdValue(UdpSocketFactory::GetTypeId()));
        onOff.SetAttribute("Remote", AddressValue(dst));

        ApplicationContainer app = onOff.Install(clientNode);
        app.Start(Seconds(client_start_time));
        app.Stop(Seconds(client_stop_time));
    }
    // **启用路由**
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Ptr<NetDevice> netDevice0 = devices.Get(0); // host 0 port 0
    Ptr<PointToPointNetDevice> p2pNetDevice0 = DynamicCast<PointToPointNetDevice>(netDevice0);
    if (p2pNetDevice0)
    {
        NS_LOG_INFO("TraceConnectWithoutContext for host 0.");
        p2pNetDevice0->TraceConnectWithoutContext("MacTx", MakeCallback(&TxCallback_host_0));
    }

    // 启动吞吐量统计
    Simulator::Schedule(Seconds(1.0), &CalculateThroughput);

    // **启动仿真**
    Simulator::Stop(Seconds(global_stop_time));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
