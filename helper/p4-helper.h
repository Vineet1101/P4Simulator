#ifndef P4_HELPER_H
#define P4_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include <string>

namespace ns3 {

/**
 * \ingroup p4sim
 * \brief Helper class to manage P4 programmable bridge devices.
 *
 * The P4Helper class simplifies the creation, configuration, and installation
 * of P4-based bridge devices (e.g., ns3::P4SwitchNetDevice) on NS-3 nodes. It
 * allows for adding LAN segments as ports and configuring attributes for the
 * bridge devices. This helper is designed for simulations involving P4
 * programmable switches or devices.
 */
class P4Helper
{
public:
  /**
     * \brief Construct a P4Helper object.
     *
     * This initializes the internal object factory for creating
     * ns3::P4SwitchNetDevice instances.
     */
  P4Helper ();

  /**
     * \brief Set an attribute on the P4 bridge device.
     *
     * This method configures attributes for the bridge devices created by this
     * helper. The attributes can control various aspects of the bridge device's
     * behavior.
     *
     * \param name The name of the attribute to set.
     * \param value The value of the attribute to set.
     */
  void SetDeviceAttribute (const std::string &name, const AttributeValue &value);

  /**
     * \brief Install a P4 bridge device on a given node.
     *
     * This method creates an ns3::P4SwitchNetDevice with the configured
     * attributes, adds the device to the specified node, and attaches the
     * given net devices as bridge ports.
     *
     * \param node The node to install the device on.
     * \param c Container of NetDevices to add as bridge ports.
     * \return A container holding the created bridge device.
     */
  NetDeviceContainer Install (Ptr<Node> node, const NetDeviceContainer &c) const;

  /**
     * \brief Install a P4 bridge device on a named node.
     *
     * This method resolves the node by name, creates an
     * ns3::P4SwitchNetDevice with the configured attributes, adds the
     * device to the resolved node, and attaches the given net devices
     * as bridge ports.
     *
     * \param nodeName The name of the node to install the device on.
     * \param c Container of NetDevices to add as bridge ports.
     * \return A container holding the created bridge device.
     */
  NetDeviceContainer Install (const std::string &nodeName, const NetDeviceContainer &c) const;

private:
  ObjectFactory m_deviceFactory; //!< Factory for creating P4 bridge devices.
};

} // namespace ns3

#endif /* P4_HELPER_H */
