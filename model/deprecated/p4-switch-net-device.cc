#include "p4-switch-net-device.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("P4SwitchNetDevice");

NS_OBJECT_ENSURE_REGISTERED(P4SwitchNetDevice);

TypeId
P4SwitchNetDevice::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::P4SwitchNetDevice")
    .SetParent<PointToPointNetDevice>()
    .SetGroupName("Network")
    .AddConstructor<P4SwitchNetDevice>();
  return tid;
}

P4SwitchNetDevice::P4SwitchNetDevice()
  : PointToPointNetDevice(),
    m_core(nullptr) // Initialize core to nullptr
{
  NS_LOG_FUNCTION(this);
}

P4SwitchNetDevice::P4SwitchNetDevice(std::string p4JsonPath)
  : PointToPointNetDevice()
{
  NS_LOG_FUNCTION(this << p4JsonPath);
  // Initialize m_core with a P4SwitchNetDeviceCore instance and load P4 program
  m_core = new P4SwitchNetDeviceCore(p4JsonPath);
}

P4SwitchNetDevice::~P4SwitchNetDevice()
{
  NS_LOG_FUNCTION(this);
  if (m_core)
  {
    delete m_core;
    m_core = nullptr;
  }
}

void
P4SwitchNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION(this);
  m_rxCallback = cb;
}

void
P4SwitchNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION(this);
  m_promiscRxCallback = cb;
}

void
P4SwitchNetDevice::Callback(Ptr<NetDevice> device,
                            Ptr<const Packet> packet,
                            uint16_t protocol,
                            const Address& source,
                            const Address& destination,
                            PacketType packetType)
{
  NS_LOG_FUNCTION(this << packet << protocol << source << destination);

  // Example of invoking P4 core logic for processing
  if (m_core)
  {
    Ptr<Packet> processedPacket = m_core->ProcessPacket(packet);
    if (!processedPacket)
    {
      NS_LOG_WARN("Packet dropped by P4 processing");
      return;
    }

    // Deliver the processed packet
    if (!m_rxCallback.IsNull())
    {
      m_rxCallback(this, processedPacket, protocol, source);
    }
  }
  else
  {
    NS_LOG_ERROR("P4 core is not initialized");
  }
}

void
P4SwitchNetDevice::ReceivePackets(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this << packet);

  // Process incoming packets through P4 core logic
  if (m_core)
  {
    Ptr<Packet> processedPacket = m_core->ProcessPacket(packet);
    if (!processedPacket)
    {
      NS_LOG_WARN("Packet dropped by P4 processing");
      return;
    }

    // Pass the processed packet to the upper layer
    if (!m_rxCallback.IsNull())
    {
      m_rxCallback(this, processedPacket
