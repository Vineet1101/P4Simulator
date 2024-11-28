/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Stanford University
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
 * Authors: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#include "bridge-p4-net-device.h"

#include "global.h"
#include "p4-switch-interface.h"

#include "ns3/boolean.h"
#include "ns3/channel.h"
#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

/**
 * \file
 * \ingroup bridge
 * ns3::BridgeP4NetDevice implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BridgeP4NetDevice");

NS_OBJECT_ENSURE_REGISTERED(BridgeP4NetDevice);

TypeId
BridgeP4NetDevice::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BridgeP4NetDevice")
                            .SetParent<NetDevice>()
                            .SetGroupName("Bridge")
                            .AddConstructor<BridgeP4NetDevice>()
                            .AddAttribute("Mtu",
                                          "The MAC-level Maximum Transmission Unit",
                                          UintegerValue(1500),
                                          MakeUintegerAccessor(&BridgeP4NetDevice::SetMtu,
                                                               &BridgeP4NetDevice::GetMtu),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

BridgeP4NetDevice::BridgeP4NetDevice()
    : m_node(nullptr),
      m_ifIndex(0)
{
    NS_LOG_FUNCTION_NOARGS();
    m_channel = CreateObject<P4BridgeChannel>();

    m_p4Switch = new P4Switch(this);

    P4SwitchInterface* p4SwitchInterface = P4GlobalVar::g_p4Controller.AddP4Switch();
    p4SwitchInterface->SetP4NetDeviceCore(m_p4Switch); //!< Pointer to the P4 core model.
    p4SwitchInterface->SetJsonPath(
        P4GlobalVar::g_p4JsonPath); //!< Path to the P4 JSON configuration file.
    p4SwitchInterface->SetP4InfoPath(P4GlobalVar::g_p4MatchTypePath); //!< Path to the P4 info file.
    p4SwitchInterface->SetFlowTablePath(
        P4GlobalVar::g_flowTablePath); //!< Path to the flow table file.
    p4SwitchInterface->SetViewFlowTablePath(
        P4GlobalVar::g_viewFlowTablePath); //!< Path to the view flow table file.
    p4SwitchInterface->SetNetworkFunc(P4GlobalVar::g_networkFunc); //!< Network function ID.
    p4SwitchInterface->SetPopulateFlowTableWay(
        P4GlobalVar::g_populateFlowTableWay); //!< Method to populate the flow table.

    // std::string jsonPath = P4GlobalVar::g_p4JsonPath;
    // std::vector<char*> args;
    // args.push_back(nullptr);
    // args.push_back(jsonPath.data());
    // m_p4Switch->init(static_cast<int>(args.size()), args.data());

    p4SwitchInterface->Init(); // init the switch with p4 configure files (*.json)
    
    m_p4Switch->start_and_return_();

    // // Init P4Model Flow Table
    // if (P4GlobalVar::g_populateFlowTableWay == LOCAL_CALL)
    //     p4SwitchInterface->Init();

    // Clear the local pointer to avoid accidental use; the object is managed by P4Controller
    p4SwitchInterface = nullptr;

    NS_LOG_LOGIC("A P4 Netdevice was initialized.");
}

BridgeP4NetDevice::~BridgeP4NetDevice()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
BridgeP4NetDevice::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        *iter = nullptr;
    }
    m_ports.clear();
    m_channel = nullptr;
    m_node = nullptr;
    NetDevice::DoDispose();
}

void
BridgeP4NetDevice::ReceiveFromDevice(Ptr<NetDevice> incomingPort,
                                     Ptr<const Packet> packet,
                                     uint16_t protocol,
                                     const Address& src,
                                     const Address& dst,
                                     PacketType packetType)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_DEBUG("UID is " << packet->GetUid());

    Mac48Address src48 = Mac48Address::ConvertFrom(src);
    Mac48Address dst48 = Mac48Address::ConvertFrom(dst);

    if (!m_promiscRxCallback.IsNull())
    {
        m_promiscRxCallback(this, packet, protocol, src, dst, packetType);
    }

    if (dst48 == m_address)
    {
        m_rxCallback(this, packet, protocol, src);
    }

    int inPort = GetPortNumber(incomingPort);

    EthernetHeader eeh;
    eeh.SetDestination(dst48);
    eeh.SetSource(src48);
    eeh.SetLengthType(protocol);

    Ptr<ns3::Packet> ns3Packet((ns3::Packet*)PeekPointer(packet));
    ns3Packet->AddHeader(eeh);

    m_p4Switch->ReceivePacket(ns3Packet, inPort, protocol, dst);
}

uint32_t
BridgeP4NetDevice::GetNBridgePorts() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports.size();
}

Ptr<NetDevice>
BridgeP4NetDevice::GetBridgePort(uint32_t n) const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ports[n];
}

void
BridgeP4NetDevice::AddBridgePort(Ptr<NetDevice> bridgePort)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(bridgePort != this);
    if (!Mac48Address::IsMatchingType(bridgePort->GetAddress()))
    {
        NS_FATAL_ERROR("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
    if (!bridgePort->SupportsSendFrom())
    {
        NS_FATAL_ERROR("Device does not support SendFrom: cannot be added to bridge.");
    }
    if (m_address == Mac48Address())
    {
        m_address = Mac48Address::ConvertFrom(bridgePort->GetAddress());
    }

    NS_LOG_DEBUG("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId().GetName());

    m_node->RegisterProtocolHandler(MakeCallback(&BridgeP4NetDevice::ReceiveFromDevice, this),
                                    0,
                                    bridgePort,
                                    true);
    m_ports.push_back(bridgePort);
    m_channel->AddChannel(bridgePort->GetChannel());
}

uint32_t
BridgeP4NetDevice::GetPortNumber(Ptr<NetDevice> port) const
{
    int portsNum = GetNBridgePorts();
    for (int i = 0; i < portsNum; i++)
    {
        if (GetBridgePort(i) == port)
            NS_LOG_DEBUG("Port found: " << i);
        return i;
    }
    NS_LOG_ERROR("Port not found");
    return -1;
}

void
BridgeP4NetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION_NOARGS();
    m_ifIndex = index;
}

uint32_t
BridgeP4NetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_ifIndex;
}

Ptr<Channel>
BridgeP4NetDevice::GetChannel() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_channel;
}

void
BridgeP4NetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION_NOARGS();
    m_address = Mac48Address::ConvertFrom(address);
}

Address
BridgeP4NetDevice::GetAddress() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_address;
}

bool
BridgeP4NetDevice::SetMtu(const uint16_t mtu)
{
    NS_LOG_FUNCTION_NOARGS();
    m_mtu = mtu;
    return true;
}

uint16_t
BridgeP4NetDevice::GetMtu() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_mtu;
}

bool
BridgeP4NetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

void
BridgeP4NetDevice::AddLinkChangeCallback(Callback<void> callback)
{
}

bool
BridgeP4NetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
BridgeP4NetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return Mac48Address::GetBroadcast();
}

bool
BridgeP4NetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

Address
BridgeP4NetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this << multicastGroup);
    Mac48Address multicast = Mac48Address::GetMulticast(multicastGroup);
    return multicast;
}

bool
BridgeP4NetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

bool
BridgeP4NetDevice::IsBridge() const
{
    NS_LOG_FUNCTION_NOARGS();
    return true;
}

bool
BridgeP4NetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION_NOARGS();
    return SendFrom(packet, m_address, dest, protocolNumber);
}

bool
BridgeP4NetDevice::SendFrom(Ptr<Packet> packet,
                            const Address& src,
                            const Address& dest,
                            uint16_t protocolNumber)
{
    /*
     */
    NS_LOG_FUNCTION_NOARGS();
    Mac48Address dst = Mac48Address::ConvertFrom(dest);

    // try to use the learned state if data is unicast
    // if (!dst.IsGroup())
    // {
    //     Ptr<NetDevice> outPort = GetLearnedState(dst);
    //     if (outPort)
    //     {
    //         outPort->SendFrom(packet, src, dest, protocolNumber);
    //         return true;
    //     }
    // }

    // 广播数据包
    // data was not unicast or no state has been learned for that mac
    // address => flood through all ports.
    Ptr<Packet> pktCopy;
    for (auto iter = m_ports.begin(); iter != m_ports.end(); iter++)
    {
        pktCopy = packet->Copy();
        Ptr<NetDevice> port = *iter;
        port->SendFrom(pktCopy, src, dest, protocolNumber);
    }

    return true;
}

void
BridgeP4NetDevice::SendPacket(Ptr<Packet> packetOut,
                              int outPort,
                              uint16_t protocol,
                              const Address& destination)
{
    // this->SendNs3Packet(packetOut, outPort, protocol, destination);
    return;
}

void
BridgeP4NetDevice::SendNs3Packet(Ptr<Packet> packetOut,
                                 int outPort,
                                 uint16_t protocol,
                                 const Address& destination)
{
    NS_LOG_DEBUG("Sending ns3 packet to port " << outPort);
    if (packetOut)
    {
        EthernetHeader eeh;
        packetOut->RemoveHeader(eeh);

        if (outPort != 511)
        {
            NS_LOG_DEBUG("EgressPortNum: " << outPort);
            Ptr<NetDevice> outNetDevice = GetBridgePort(outPort);
            outNetDevice->Send(packetOut->Copy(), destination, protocol);
        }
    }
    else
        NS_LOG_DEBUG("Null Packet!");
}

Ptr<Node>
BridgeP4NetDevice::GetNode() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_node;
}

void
BridgeP4NetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION_NOARGS();
    m_node = node;
}

bool
BridgeP4NetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

void
BridgeP4NetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_rxCallback = cb;
}

void
BridgeP4NetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION_NOARGS();
    m_promiscRxCallback = cb;
}

bool
BridgeP4NetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

Address
BridgeP4NetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    return Mac48Address::GetMulticast(addr);
}

} // namespace ns3
