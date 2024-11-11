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

#ifndef BRIDGE_P4_NET_DEVICE_H
#define BRIDGE_P4_NET_DEVICE_H

#include "bridge-channel.h"

#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/nstime.h"

#include <map>
#include <stdint.h>
#include <string>

/**
 * \file
 * \ingroup bridge
 * ns3::BridgeP4NetDevice declaration.
 */

namespace ns3
{

class Node;

/**
 * \defgroup bridge Bridge P4 Network Device
 *
 * \brief A Bridge Net Device with Programmable Data Plane
 *
 * P4NetDevice is a subclass of NetDevice in the ns-3 domain and serves as
 * the network layer of a P4 target. It is compatible with other net devices
 * in ns-3.
 *
 * \attention The Spanning Tree Protocol part of 802.1D is not
 * implemented.  Therefore, you have to be careful not to create
 * bridging loops, or else the network will collapse.
 *
 * \attention Bridging is designed to work only with NetDevices
 * modelling IEEE 802-style technologies, such as CsmaNetDevice and
 * WifiNetDevice.
 * 
 * \TODO Create a new channel class supporting arbitrary underlying channel.
 */

/**
 * \ingroup bridge
 * \brief A Bridge Net Device with Programmable Data Plane, based on that,
 * it also can be a p4 switch in network.
 */
class BridgeP4NetDevice : public NetDevice
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    BridgeP4NetDevice();
    ~BridgeP4NetDevice() override;

    // Delete copy constructor and assignment operator to avoid misuse
    BridgeP4NetDevice(const BridgeP4NetDevice&) = delete;
    BridgeP4NetDevice& operator=(const BridgeP4NetDevice&) = delete;

    /**
     * \brief Add a 'port' to a P4 bridge device
     * \param bridgePort the NetDevice to add
     *
     * This method adds a new bridge port to a BridgeP4NetDevice, so that
     * the new bridge port NetDevice becomes part of the bridge and L2
     * frames start being forwarded to/from this NetDevice.
     *
     * \attention The netdevice that is being added as bridge port must
     * _not_ have an IP address.  In order to add IP connectivity to a
     * bridging node you must enable IP on the BridgeNetDevice itself,
     * never on its port netdevices.
     */
    void AddBridgePort(Ptr<NetDevice> bridgePort);

    /**
     * \brief Gets the number of bridged 'ports', i.e., the NetDevices currently bridged.
     *
     * \return the number of bridged ports.
     */
    uint32_t GetNBridgePorts() const;

    /**
    * \brief Gets the number ID of a 'port' connected to P4 net device.
    * \param netdevice
    * \return the port number of the p4 bridge device
    */
    uint32_t GetPortNumber(Ptr<NetDevice> port) const;

    /**
     * \brief Gets the n-th bridged port.
     * \param n the port index
     * \return the n-th bridged NetDevice
     */
    Ptr<NetDevice> GetBridgePort(uint32_t n) const;

    /**
    * \brief This method sends a packet out to the destination port.
    * \param packetOut the packet need to be send out
    * \param outPort the port index to send out
    * \param protocol the packet protocol (e.g., Ethertype)
    * \param destination the packet destination
    */
    void SendPacket(Ptr<Packet> packetOut, int outPort, uint16_t protocol, Address const &destination);


    void SendNs3Packet(Ptr<Packet> packetOut, int outPort, uint16_t protocol, Address const &destination);
    
    // inherited from NetDevice base class.
    void SetIfIndex(const uint32_t index) override;
    uint32_t GetIfIndex() const override;
    Ptr<Channel> GetChannel() const override;
    void SetAddress(Address address) override;
    Address GetAddress() const override;
    bool SetMtu(const uint16_t mtu) override;
    uint16_t GetMtu() const override;
    bool IsLinkUp() const override;
    void AddLinkChangeCallback(Callback<void> callback) override;
    bool IsBroadcast() const override;
    Address GetBroadcast() const override;
    bool IsMulticast() const override;
    Address GetMulticast(Ipv4Address multicastGroup) const override;
    bool IsPointToPoint() const override;
    bool IsBridge() const override;
    bool Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber) override;
    bool SendFrom(Ptr<Packet> packet,
                  const Address& source,
                  const Address& dest,
                  uint16_t protocolNumber) override;
    Ptr<Node> GetNode() const override;
    void SetNode(Ptr<Node> node) override;
    bool NeedsArp() const override;
    void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
    void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;
    bool SupportsSendFrom() const override;
    Address GetMulticast(Ipv6Address addr) const override;

  protected:
    void DoDispose() override;

    /**
     * \brief Receives a packet from one bridged port.
     * \param device the originating port
     * \param packet the received packet
     * \param protocol the packet protocol (e.g., Ethertype)
     * \param source the packet source
     * \param destination the packet destination
     * \param packetType the packet type (e.g., host, broadcast, etc.)
     */
    void ReceiveFromDevice(Ptr<NetDevice> device,
                           Ptr<const Packet> packet,
                           uint16_t protocol,
                           const Address& source,
                           const Address& destination,
                           PacketType packetType);


    // /**
    //  * \brief Gets the port associated to a source address
    //  * \param source the source address
    //  * \returns the port the source is associated to, or NULL if no association is known.
    //  */
    // Ptr<NetDevice> GetLearnedState(Mac48Address source);

  private:
    NetDevice::ReceiveCallback m_rxCallback;               //!< receive callback
    NetDevice::PromiscReceiveCallback m_promiscRxCallback; //!< promiscuous receive callback

    Mac48Address m_address; //!< MAC address of the NetDevice
    Ptr<Node> m_node;                                  //!< node owning this NetDevice
    Ptr<BridgeChannel> m_channel;                      //!< virtual bridged channel
    std::vector<Ptr<NetDevice>> m_ports;               //!< bridged ports
    uint32_t m_ifIndex;                                //!< Interface index
    uint16_t m_mtu;                                    //!< MTU of the bridged NetDevice


};

} // namespace ns3

#endif /* BRIDGE_P4_NET_DEVICE_H */
