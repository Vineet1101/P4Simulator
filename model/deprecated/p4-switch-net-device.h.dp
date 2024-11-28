#ifndef P4_SWITCH_NET_DEVICE_H
#define P4_SWITCH_NET_DEVICE_H

#include "ns3/point-to-point-net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"

#include "p4-switch-net-device-core.h"

#include <string>

namespace ns3 {

class P4SwitchNetDeviceCore;

class P4SwitchNetDevice : public PointToPointNetDevice
{
public:
  static TypeId GetTypeId(void);

  P4SwitchNetDevice();
  P4SwitchNetDevice(std::string p4JsonPath);
  virtual ~P4SwitchNetDevice();

  // Override from NetDevice
  virtual void SetReceiveCallback(NetDevice::ReceiveCallback cb) override;
  virtual void SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb) override;

  // Custom callback for packet processing
  void Callback(Ptr<NetDevice> device,
                Ptr<const Packet> packet,
                uint16_t protocol,
                const Address& source,
                const Address& destination,
                PacketType packetType);

  // Handle received packets
  void ReceivePackets(Ptr<Packet> packet);

private:
  // Callbacks for packet reception
  NetDevice::ReceiveCallback m_rxCallback;
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  // Core logic for P4 processing
  P4SwitchNetDeviceCore* m_core; // Pointer to P4 processing core
};

} // namespace ns3

#endif // P4_SWITCH_NET_DEVICE_H
