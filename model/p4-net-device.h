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
 * 
 * Author: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#ifndef P4_NET_DEVICE_H
#define P4_NET_DEVICE_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/ptr.h"

namespace ns3
{

class P4NetDeviceCore;

class P4NetDevice : public PointToPointNetDevice
{
  public:
    static TypeId GetTypeId(void);

    P4NetDevice();
    P4NetDevice(std::string p4_json_path);

    virtual ~P4NetDevice();

    void Callback(Ptr<NetDevice> device,
                  Ptr<const Packet> packet,
                  uint16_t protocol,
                  const Address& source,
                  const Address& destination,
                  PacketType packetType);

    virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb);
    virtual void SetPromiscReceiveCallback(
			NetDevice::PromiscReceiveCallback cb);

    void ReceivePackets(Ptr<Packet> packet);

  private:
    
    NetDevice::ReceiveCallback m_rxCallback; 					//!< receive callback
    NetDevice::PromiscReceiveCallback m_promiscRxCallback; 		//!< promiscuous receive callback

    P4NetDeviceCore* m_core;
};

} // namespace ns3

#endif // P4_NET_DEVICE_H
