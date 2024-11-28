/*
* Copyright (c) YEAR COPYRIGHTHOLDER
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
* Author: Mingyu Ma <mingyu.ma@tu-dresden.de>
*/


#include "p4-net-device.h"
#include "p4-net-device-core.h"

// #include "ppp-header.h"

namespace ns3 {

TypeId P4NetDevice::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::P4NetDevice")
        .SetParent<PointToPointNetDevice> ()  
        .SetGroupName ("Network"); 
}


P4NetDevice::P4NetDevice ()
    : PointToPointNetDevice (),
      m_core(nullptr) 
{
    // Default will be a P2P NetDevice
    NS_LOG_FUNCTION (this);
    NS_LOG_WARN("Warn: need a p4 json path to initialize a P4 Net device.");
}

P4NetDevice::P4NetDevice (std::string p4_json_path)
    : PointToPointNetDevice (),
      m_core(nullptr) 
{
    NS_LOG_FUNCTION (this);

    m_core = new P4NetDeviceCore(this);

    char * a3 = (char*)p4_json_path.data();
    char * args[2] = { nullptr, a3 };

    m_core->init(2, args);
    m_core->start_and_return_();

    NS_LOG_LOGIC("A P4 Net device with P4 core was initialized.");
}

P4NetDevice::~P4NetDevice ()
{
    NS_LOG_FUNCTION (this);

    if (m_core != nullptr)
    {
        delete m_core;
        m_core = nullptr;
    }
}

void
P4NetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb) {
	NS_LOG_FUNCTION(&cb);
    m_rxCallback = cb;
}

void
P4NetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) {
    NS_LOG_FUNCTION(&cb);
    m_promiscRxCallback = cb;
}



void 
P4NetDevice::ReceivePackets(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    
    // @TODO here we remove the protocol and the destination address
    uint16_t protocol = 0;
    // ProcessHeader(packet, protocol);

    if (m_core)
    {
        NS_LOG_LOGIC("Processing packet through P4NetDeviceCore.");
        m_core->ReceivePacket(packet, GetIfIndex(), protocol, Address());
    }
    else
    {
        NS_LOG_WARN("P4NetDeviceCore is not initialized. Packet will be dropped.");
        return; // Fix: Remove the return value as the function has void return type
    }
}

} // namespace ns3
