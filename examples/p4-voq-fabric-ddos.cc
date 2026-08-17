/*
 * Copyright (c) 2025 TU Dresden
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Vineet Goel <vineetgoel692@gmail.com>
 */

/**
 * DDoS-mitigation demonstration for the VOQ + fabric Traffic Manager.
 *
 * A single legitimate client shares a bottleneck link to a server with two
 * attacker hosts that flood the same victim.  The switch runs the `qos` P4
 * program, which classifies packets by UDP destination port and writes the
 * priority the Traffic Manager schedules on.
 *
 *   host0 legit    (10.1.1.1, port 0) ── dport 4000 ─┐
 *   host2 attacker (10.1.1.3, port 2) ── dport 5000 ─┤
 *   host3 attacker (10.1.1.4, port 3) ── dport 5000 ─┴─► switch ─►[bottleneck]─► host1 victim
 *                                                                                (10.1.1.2, port 1)
 *
 * Every sender enters on its own ingress port (its own host NIC and VOQ), so
 * the flood and the legitimate flow contend only inside the switch at the shared
 * output port 1, which is the bottleneck.  The two attackers together offer far
 * more than the link can carry, so without protection the legitimate client is
 * crowded out.
 *
 * Two modes select whether the legitimate flow is protected (same flood in both;
 * bmv2 cannot be re-initialised within one process, so one mode per run):
 *
 *   --mitigate=true   (default) legit dport 4000 is marked priority 3; strict
 *                     priority protects it and the flood (priority 0) is dropped.
 *   --mitigate=false  no marking (legit stays priority 0 like the flood); the
 *                     legitimate flow gets only its fair share and is degraded.
 *
 *   ./ns3 run "p4-voq-fabric-ddos --mitigate=true"
 *   ./ns3 run "p4-voq-fabric-ddos --mitigate=false"
 *
 * Exit code 0 = the mode's expected outcome held; non-zero = it did not.
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/data-rate.h"
#include "ns3/format-utils.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/p4-core-v1model.h"
#include "ns3/p4-helper.h"
#include "ns3/p4-switch-net-device.h"
#include "ns3/p4-traffic-manager.h"
#include "ns3/packet-sink.h"
#include "ns3/switched-ethernet-channel.h"
#include "ns3/switched-ethernet-helper.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P4VoqFabricDdos");

namespace
{

int g_failures = 0;

void
Check(bool cond, const std::string& what)
{
    std::cout << "  [" << (cond ? "PASS" : "FAIL") << "] " << what << "\n";
    if (!cond)
    {
        ++g_failures;
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    bool mitigate = true;               // mark + protect the legitimate flow
    std::string egressLink = "100Mbps"; // bottleneck: switch(port 1) -> victim
    std::string ingressLink = "10Gbps"; // kept fast: senders -> switch
    uint32_t pktSize = 1400;            // UDP payload bytes
    double legitFactor = 0.4;           // legit offered  = factor * egress line
    double attackFactor = 1.0;          // each attacker  = factor * egress line
    double flowDuration = 0.2;          // seconds of traffic
    uint32_t egressBufferBytes = 65536; // finite per-priority egress queue

    CommandLine cmd;
    cmd.AddValue("mitigate", "Mark and protect the legitimate flow (else no marking)", mitigate);
    cmd.AddValue("egressLink", "Bottleneck egress link rate switch->victim", egressLink);
    cmd.AddValue("ingressLink", "Ingress link rate senders->switch (kept fast)", ingressLink);
    cmd.AddValue("pktSize", "UDP payload size in bytes", pktSize);
    cmd.AddValue("legitFactor", "Legit offered load as a multiple of the egress line", legitFactor);
    cmd.AddValue("attackFactor", "Per-attacker offered load as a multiple of the egress line", attackFactor);
    cmd.AddValue("flowDuration", "Duration of the flows (s)", flowDuration);
    cmd.AddValue("egressBufferBytes", "Per-priority egress queue limit in bytes (0 = unlimited)", egressBufferBytes);
    cmd.Parse(argc, argv);

    const uint64_t egressBps = DataRate(egressLink).GetBitRate();
    auto rateStr = [](uint64_t bps) {
        std::ostringstream o;
        o << bps << "bps";
        return o.str();
    };
    const std::string legitRate = rateStr(static_cast<uint64_t>(egressBps * legitFactor));
    const std::string attackRate = rateStr(static_cast<uint64_t>(egressBps * attackFactor));

    std::cout << "=== VOQ+fabric DDoS-mitigation demo ===\n"
              << "  mode=" << (mitigate ? "MITIGATE (legit marked prio 3)" : "NO MARKING")
              << "  egressLink=" << egressLink << "\n"
              << "  legit=" << legitFactor << "x line   2 attackers @ " << attackFactor
              << "x line each (flood " << (2 * attackFactor) << "x)\n";

    // Per-PRIORITY egress queue limit (not a shared per-port pool): each
    // priority level gets its own bounded buffer, so a low-priority flood
    // overflows only its own queue and cannot exhaust the buffer that
    // high-priority traffic needs to be admitted.  This queue isolation is what
    // lets strict priority actually protect the marked flow under a flood.
    if (egressBufferBytes > 0)
    {
        Config::SetDefault("ns3::P4TrafficManager::EgressQueueLimit",
                           UintegerValue(egressBufferBytes));
    }

    // ---- Topology: legit(0), victim(1), attacker(2), attacker(3) ----
    NodeContainer terminals;
    terminals.Create(4);
    Ptr<Node> switchNode = CreateObject<Node>();

    InternetStackHelper internet;
    internet.Install(terminals);
    internet.Install(switchNode);

    Ipv4AddressHelper ipv4Addr;
    ipv4Addr.SetBase("10.1.1.0", "255.255.255.0");

    const std::string p4Dir = GetP4ExamplePath() + "/qos";
    // Same flood in both modes; the only difference is whether the flowtable
    // marks the legitimate flow's UDP port with a high priority.
    const std::string flowTable =
        p4Dir + (mitigate ? "/flowtable_ddos.txt" : "/flowtable_ddos_nomark.txt");

    P4Helper p4;
    p4.SetDeviceAttribute("JsonPath", StringValue(p4Dir + "/qos.json"));
    p4.SetDeviceAttribute("FlowTablePath", StringValue(flowTable));
    p4.SetDeviceAttribute("P4SwitchArch", UintegerValue(0)); // V1model
    p4.SetDeviceAttribute("SwitchRate", UintegerValue(10000));
    p4.SetDeviceAttribute("EnableVoqFabric", BooleanValue(true));
    Ptr<P4SwitchNetDevice> sw = DynamicCast<P4SwitchNetDevice>(p4.Install(switchNode).Get(0));

    SwitchedEthernetHelper eth;
    eth.SetChannelAttribute("DataRate", StringValue(ingressLink));
    eth.SetChannelAttribute("Delay", StringValue("1us"));
    NetDeviceContainer hostDevs = eth.Install(sw, terminals);

    // MACs match the qos flowtable's forwarding rewrite (dst IP -> dst MAC/port):
    //   10.1.1.1 -> port 0 ...:01 (legit)   10.1.1.2 -> port 1 ...:03 (victim)
    //   10.1.1.3 -> port 2 ...:05 (atk1)    10.1.1.4 -> port 3 ...:07 (atk2)
    const char* macs[4] = {"00:00:00:00:00:01",
                           "00:00:00:00:00:03",
                           "00:00:00:00:00:05",
                           "00:00:00:00:00:07"};
    for (uint32_t i = 0; i < terminals.GetN(); ++i)
    {
        hostDevs.Get(i)->SetAddress(Mac48Address(macs[i]));
        ipv4Addr.Assign(hostDevs.Get(i));
    }

    // Bottleneck: only the victim's egress link (port 1) is slow.
    sw->GetPortChannel(1)->SetAttribute("DataRate", DataRateValue(DataRate(egressLink)));

    // ---- Flows: all -> victim (host1) ----
    const uint16_t legitPort = 4000; // marked prio 3 when mitigating
    const uint16_t attackPort = 5000; // never marked -> prio 0
    Ipv4Address victimAddr = terminals.Get(1)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

    PacketSinkHelper legitSink("ns3::UdpSocketFactory",
                               InetSocketAddress(Ipv4Address::GetAny(), legitPort));
    PacketSinkHelper attackSink("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), attackPort));
    ApplicationContainer legitSinkApp = legitSink.Install(terminals.Get(1));
    ApplicationContainer attackSinkApp = attackSink.Install(terminals.Get(1));
    legitSinkApp.Start(Seconds(1.0));
    attackSinkApp.Start(Seconds(1.0));
    legitSinkApp.Stop(Seconds(2.0 + flowDuration + 1.0));
    attackSinkApp.Stop(Seconds(2.0 + flowDuration + 1.0));

    auto makeFlow = [&](uint32_t senderIdx, uint16_t dport, const std::string& rate) {
        OnOffHelper onOff("ns3::UdpSocketFactory", InetSocketAddress(victimAddr, dport));
        onOff.SetAttribute("PacketSize", UintegerValue(pktSize));
        onOff.SetAttribute("DataRate", StringValue(rate));
        onOff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        ApplicationContainer app = onOff.Install(terminals.Get(senderIdx));
        app.Start(Seconds(2.0));
        app.Stop(Seconds(2.0 + flowDuration));
    };
    makeFlow(0, legitPort, legitRate);   // legitimate client
    makeFlow(2, attackPort, attackRate); // attacker 1
    makeFlow(3, attackPort, attackRate); // attacker 2

    Simulator::Stop(Seconds(2.0 + flowDuration + 1.0));
    Simulator::Run();

    // ---- Capture results while the core / TM still exist ----
    const uint64_t legitRx = DynamicCast<PacketSink>(legitSinkApp.Get(0))->GetTotalRx();
    const uint64_t attackRx = DynamicCast<PacketSink>(attackSinkApp.Get(0))->GetTotalRx();

    uint64_t tmDropped = 0;
    P4CoreV1model* core = sw->GetV1ModelCore();
    Ptr<P4TrafficManager> tm = core ? core->GetTrafficManager() : nullptr;
    if (tm)
    {
        tmDropped = tm->GetStats().totalDropped;
    }

    Simulator::Destroy();

    // ---- Results ----
    const double legitMbps = legitRx * 8.0 / flowDuration / 1e6;
    const double attackMbps = attackRx * 8.0 / flowDuration / 1e6;
    const double legitOfferedMbps = egressBps * legitFactor / 1e6;
    const double legitRetained = (legitOfferedMbps > 0) ? (legitMbps / legitOfferedMbps) : 0.0;

    std::cout << std::fixed << std::setprecision(2)
              << "  LEGIT : rx=" << legitRx << " B  ~" << legitMbps << " Mbps  (offered ~"
              << legitOfferedMbps << " Mbps, retained " << (legitRetained * 100.0) << "%)\n"
              << "  FLOOD : rx=" << attackRx << " B  ~" << attackMbps << " Mbps\n"
              << "  [TM] dropped=" << tmDropped << "\n";

    Check(tm != nullptr, "Traffic Manager active on the switch");
    Check(tmDropped > 0, "Bottleneck oversubscribed (Traffic Manager dropped excess)");
    // Priority protects the legitimate flow's *own* offered load; it does not
    // make legit out-total a two-attacker flood (whose aggregate leftover is
    // naturally larger).  So the meaningful test is how much of its offered load
    // the legit flow keeps: near-full when marked, only a degraded share when
    // not.  Run both modes to see the gap.
    if (mitigate)
    {
        Check(legitRetained >= 0.90,
              "MITIGATE: legitimate flow protected under flood (kept ~all offered load)");
    }
    else
    {
        Check(legitRetained <= 0.80,
              "NO MARKING: legitimate flow degraded by the flood (fair-share only)");
    }

    const bool ok = (g_failures == 0);
    std::cout << "=== "
              << (ok ? (mitigate ? "LEGIT PROTECTED" : "LEGIT DEGRADED (as expected without marking)")
                     : "UNEXPECTED OUTCOME")
              << " (" << g_failures << " failure(s)) ===\n";
    return ok ? 0 : 1;
}
