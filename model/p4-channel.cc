/*
 * Copyright (c) 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This implementation uses a customized version of the Point-to-Point Channel 
 * to integrate with the P4-Net-Device. The reason for copying and modifying the 
 * Point-to-Point Channel code is that the default Point-to-Point Net Device 
 * does not allow overriding its receive and processing functions, which are 
 * essential for P4-based packet handling. Additionally, the TransmitStart 
 * function in the original Point-to-Point Channel cannot be overridden via 
 * inheritance. 
 *
 * To address these limitations, the Point-to-Point Channel code was replicated 
 * and adjusted to allow the necessary customizations while preserving the 
 * original logic and behavior of the channel. This ensures compatibility with 
 * P4-Net-Device without introducing major changes to the core design.
 * 
 */

#include "p4-channel.h"

#include "p4-net-device.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("P4Channel");

NS_OBJECT_ENSURE_REGISTERED(P4Channel);

TypeId
P4Channel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::P4Channel")
            .SetParent<Channel>()
            .SetGroupName("PointToPoint")
            .AddConstructor<P4Channel>()
            .AddAttribute("Delay",
                          "Propagation delay through the channel",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&P4Channel::m_delay),
                          MakeTimeChecker())
            .AddTraceSource("TxRxPointToPoint",
                            "Trace source indicating transmission of packet "
                            "from the P4Channel, used by the Animation "
                            "interface.",
                            MakeTraceSourceAccessor(&P4Channel::m_txrxPointToPoint),
                            "ns3::P4Channel::TxRxAnimationCallback");
    return tid;
}

//
// By default, you get a channel that
// has an "infitely" fast transmission speed and zero delay.
P4Channel::P4Channel()
    : Channel(),
      m_delay(Seconds(0.)),
      m_nDevices(0)
{
    NS_LOG_FUNCTION_NOARGS();
}

void
P4Channel::Attach(Ptr<P4NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Only two devices permitted");
    NS_ASSERT(device);

    m_link[m_nDevices++].m_src = device;
    //
    // If we have both devices connected to the channel, then finish introducing
    // the two halves and set the links to IDLE.
    //
    if (m_nDevices == N_DEVICES)
    {
        m_link[0].m_dst = m_link[1].m_src;
        m_link[1].m_dst = m_link[0].m_src;
        m_link[0].m_state = IDLE;
        m_link[1].m_state = IDLE;
    }
}

bool
P4Channel::TransmitStart(Ptr<const Packet> p, Ptr<P4NetDevice> src, Time txTime)
{
    NS_LOG_FUNCTION(this << p << src);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

    NS_ASSERT(m_link[0].m_state != INITIALIZING);
    NS_ASSERT(m_link[1].m_state != INITIALIZING);

    uint32_t wire = src == m_link[0].m_src ? 0 : 1;

    Simulator::ScheduleWithContext(m_link[wire].m_dst->GetNode()->GetId(),
                                   txTime + m_delay,
                                   &P4NetDevice::Receive,
                                   m_link[wire].m_dst,
                                   p->Copy());

    // Call the tx anim callback on the net device
    m_txrxPointToPoint(p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
    return true;
}

std::size_t
P4Channel::GetNDevices() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_nDevices;
}

Ptr<P4NetDevice>
P4Channel::GetPointToPointDevice(std::size_t i) const
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT(i < 2);
    return m_link[i].m_src;
}

Ptr<NetDevice>
P4Channel::GetDevice(std::size_t i) const
{
    NS_LOG_FUNCTION_NOARGS();
    return GetPointToPointDevice(i);
}

Time
P4Channel::GetDelay() const
{
    return m_delay;
}

Ptr<P4NetDevice>
P4Channel::GetSource(uint32_t i) const
{
    return m_link[i].m_src;
}

Ptr<P4NetDevice>
P4Channel::GetDestination(uint32_t i) const
{
    return m_link[i].m_dst;
}

bool
P4Channel::IsInitialized() const
{
    NS_ASSERT(m_link[0].m_state != INITIALIZING);
    NS_ASSERT(m_link[1].m_state != INITIALIZING);
    return true;
}

} // namespace ns3
