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
 * Authors: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"
#include "ns3/node.h"
#include "ns3/p4-helper.h"
#include "ns3/p4-switch-net-device.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("P4Helper");

P4Helper::P4Helper()
{
    NS_LOG_FUNCTION(this);
    // Set the default type of the device to P4SwitchNetDevice
    m_deviceFactory.SetTypeId("ns3::P4SwitchNetDevice");
}

void
P4Helper::SetDeviceAttribute(const std::string& name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(this << name);
    m_deviceFactory.Set(name, value);
}

NetDeviceContainer
P4Helper::Install(Ptr<Node> node, const NetDeviceContainer& netDevices) const
{
    NS_ASSERT_MSG(node != nullptr, "Invalid node pointer passed to P4Helper::Install");
    uint32_t deviceCount = netDevices.GetN();
    if (deviceCount == 0)
    {
        NS_LOG_WARN("P4Helper::Install received an empty NetDeviceContainer. Returning without "
                    "installing.");
        return NetDeviceContainer();
    }

    NS_LOG_FUNCTION(this << node->GetId());
    NS_LOG_LOGIC("Installing P4SwitchNetDevice on node: " << node->GetId());

    // Create the P4 bridge device
    Ptr<P4SwitchNetDevice> switchDevice = m_deviceFactory.Create<P4SwitchNetDevice>();
    node->AddDevice(switchDevice);

    // Attach each NetDevice in the container to the bridge
    for (auto iter = netDevices.Begin(); iter != netDevices.End(); ++iter)
    {
        NS_LOG_LOGIC("Adding bridge port: " << *iter);
        switchDevice->AddBridgePort(*iter);
    }

    // Return a container with the created device
    NetDeviceContainer installedDevices;
    installedDevices.Add(switchDevice);
    return installedDevices;
}

NetDeviceContainer
P4Helper::Install(const std::string& nodeName, const NetDeviceContainer& netDevices) const
{
    NS_LOG_FUNCTION(this << nodeName);
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node, netDevices);
}

} // namespace ns3
