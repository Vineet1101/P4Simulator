#include "ns3/p4-helper.h"
#include "ns3/log.h"
#include "ns3/bridge-p4-net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4Helper");

P4Helper::P4Helper ()
{
  NS_LOG_FUNCTION (this);
  // Set the default type of the device to BridgeP4NetDevice
  m_deviceFactory.SetTypeId ("ns3::BridgeP4NetDevice");
}

void
P4Helper::SetDeviceAttribute (const std::string &name, const AttributeValue &value)
{
  NS_LOG_FUNCTION (this << name);
  m_deviceFactory.Set (name, value);
}

NetDeviceContainer
P4Helper::Install (Ptr<Node> node, const NetDeviceContainer &netDevices) const
{

  NS_ASSERT_MSG (node != nullptr, "Invalid node pointer passed to P4Helper::Install");
  uint32_t deviceCount = netDevices.GetN ();
  if (deviceCount == 0)
    {
      NS_LOG_WARN (
          "P4Helper::Install received an empty NetDeviceContainer. Returning without installing.");
      return NetDeviceContainer ();
    }

  NS_LOG_FUNCTION (this << node->GetId ());
  NS_LOG_LOGIC ("Installing BridgeP4NetDevice on node: " << node->GetId ());

  // Create the P4 bridge device
  Ptr<BridgeP4NetDevice> bridgeDevice = m_deviceFactory.Create<BridgeP4NetDevice> ();
  node->AddDevice (bridgeDevice);

  // Attach each NetDevice in the container to the bridge
  for (auto iter = netDevices.Begin (); iter != netDevices.End (); ++iter)
    {
      NS_LOG_LOGIC ("Adding bridge port: " << *iter);
      bridgeDevice->AddBridgePort (*iter);
    }

  // Return a container with the created device
  NetDeviceContainer installedDevices;
  installedDevices.Add (bridgeDevice);
  return installedDevices;
}

NetDeviceContainer
P4Helper::Install (const std::string &nodeName, const NetDeviceContainer &netDevices) const
{
  NS_LOG_FUNCTION (this << nodeName);
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node, netDevices);
}

} // namespace ns3
