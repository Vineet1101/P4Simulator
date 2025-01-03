#include "custom-p2p-net-device.h"

#include "ns3/log.h"
#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomP2PNetDevice");

NS_OBJECT_ENSURE_REGISTERED (CustomP2PNetDevice);

TypeId
CustomP2PNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::CustomP2PNetDevice")
          .SetParent<NetDevice> ()
          .AddConstructor<CustomP2PNetDevice> ()
          .AddAttribute (
              "Mtu", "The MAC-level Maximum Transmission Unit", UintegerValue (DEFAULT_MTU),
              MakeUintegerAccessor (&CustomP2PNetDevice::SetMtu, &CustomP2PNetDevice::GetMtu),
              MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                         MakeMac48AddressAccessor (&CustomP2PNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute ("DataRate", "The default data rate for point to point links",
                         DataRateValue (DataRate ("32768b/s")),
                         MakeDataRateAccessor (&CustomP2PNetDevice::m_bps), MakeDataRateChecker ())
          .AddAttribute ("ReceiveErrorModel",
                         "The receiver error model used to simulate packet loss", PointerValue (),
                         MakePointerAccessor (&CustomP2PNetDevice::m_receiveErrorModel),
                         MakePointerChecker<ErrorModel> ())
          .AddAttribute ("InterframeGap", "The time to wait between packet (frame) transmissions",
                         TimeValue (Seconds (0.0)),
                         MakeTimeAccessor (&CustomP2PNetDevice::m_tInterframeGap),
                         MakeTimeChecker ())
          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute ("TxQueue", "A queue to use as the transmit queue in the device.",
                         PointerValue (), MakePointerAccessor (&CustomP2PNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())
          //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has arrived "
                           "for transmission by this device",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been dropped "
                           "by the device before transmission",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacPromiscRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a promiscuous trace,",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_macPromiscRxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has begun "
                           "transmitting over the channel",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device during transmission",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_phyTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device during reception",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_phyRxDropTrace),
                           "ns3::Packet::TracedCallback")
          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          // Note that there is really no difference between promiscuous and
          // non-promiscuous traces in a point-to-point link.
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous packet sniffer "
                           "attached to the device",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PromiscSniffer",
                           "Trace source simulating a promiscuous packet sniffer "
                           "attached to the device",
                           MakeTraceSourceAccessor (&CustomP2PNetDevice::m_promiscSnifferTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

CustomP2PNetDevice::CustomP2PNetDevice ()
    : m_txMachineState (READY), m_channel (0), m_linkUp (false), m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
}

CustomP2PNetDevice::~CustomP2PNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
CustomP2PNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << p << protocolNumber);
  // add the header @TODO
}

bool
CustomP2PNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t &param)
{
  NS_LOG_FUNCTION (this << p << param);
  // process the header @TODO
  // PppHeader ppp;
  // p->RemoveHeader (ppp);
  // param = PppToEther (ppp.GetProtocol ());
  return true;
}

void
CustomP2PNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  NetDevice::DoDispose ();
}

void
CustomP2PNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
CustomP2PNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.As (Time::S));
  m_tInterframeGap = t;
}

bool
CustomP2PNetDevice::TransmitStart (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.As (Time::S));
  Simulator::Schedule (txCompleteTime, &CustomP2PNetDevice::TransmitComplete, this);

  bool result = m_channel->TransmitStart (p, this, txTime);
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }
  return result;
}

void
CustomP2PNetDevice::TransmitComplete (void)
{
  NS_LOG_FUNCTION (this);

  //
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "CustomP2PNetDevice::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  Ptr<Packet> p = m_queue->Dequeue ();
  if (p == 0)
    {
      NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
      return;
    }

  //
  // Got another packet off of the queue, so start the transmit process again.
  //
  m_snifferTrace (p);
  m_promiscSnifferTrace (p);
  TransmitStart (p);
}

bool
CustomP2PNetDevice::Attach (Ptr<P4P2PChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

void
CustomP2PNetDevice::SetQueue (Ptr<Queue<Packet>> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
CustomP2PNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
CustomP2PNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet))
    {
      //
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else
    {
      //
      // Hit the trace hooks.  All of these hooks are in the same place in this
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      //
      m_snifferTrace (packet);
      m_promiscSnifferTrace (packet);
      m_phyRxEndTrace (packet);

      //
      // Trace sinks will expect complete packets, not packets without some of the
      // headers.
      //
      Ptr<Packet> originalPacket = packet->Copy ();

      //
      // Strip off the point-to-point protocol header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      ProcessHeader (packet, protocol);

      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (originalPacket);
          m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (),
                             NetDevice::PACKET_HOST);
        }

      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, protocol, GetRemote ());
    }
}

Ptr<Queue<Packet>>
CustomP2PNetDevice::GetQueue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
CustomP2PNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
CustomP2PNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
CustomP2PNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

Ptr<Channel>
CustomP2PNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}

//
// This is a point-to-point device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
CustomP2PNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
CustomP2PNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

bool
CustomP2PNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
CustomP2PNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
CustomP2PNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
CustomP2PNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
CustomP2PNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
CustomP2PNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
CustomP2PNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

bool
CustomP2PNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
CustomP2PNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
CustomP2PNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());

  //
  // If IsLinkUp() is false it means there is no channel to send any packet
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  //
  // Stick a point to point protocol header on the packet in preparation for
  // shoving it out the door.
  //
  AddHeader (packet, protocolNumber);

  m_macTxTrace (packet);

  //
  // We should enqueue and dequeue the packet to hit the tracing hooks.
  //
  if (m_queue->Enqueue (packet))
    {
      //
      // If the channel is ready for transition we send the packet right now
      //
      if (m_txMachineState == READY)
        {
          packet = m_queue->Dequeue ();
          m_snifferTrace (packet);
          m_promiscSnifferTrace (packet);
          bool ret = TransmitStart (packet);
          return ret;
        }
      return true;
    }

  // Enqueue may fail (overflow)

  m_macTxDropTrace (packet);
  return false;
}

bool
CustomP2PNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                              uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  NS_LOG_LOGIC ("packet =" << packet);
  NS_LOG_LOGIC ("UID is " << packet->GetUid () << ")");

  NS_ASSERT (IsLinkUp ());

  //
  // Only transmit if send side of net device is enabled
  //
  // if (IsSendEnabled () == false)
  //   {
  //     m_macTxDropTrace (packet);
  //     return false;
  //   }

  Mac48Address mac_destination = Mac48Address::ConvertFrom (dest);
  Mac48Address mac_source = Mac48Address::ConvertFrom (source);
  NS_LOG_LOGIC ("source=" << mac_source << ", dest=" << mac_destination);

  // AddHeader (packet, source, destination, protocolNumber); // @TODO

  m_macTxTrace (packet);

  //
  // Place the packet to be sent on the send queue.  Note that the
  // queue may fire a drop trace, but we will too.
  //
  if (m_queue->Enqueue (packet) == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  //
  // If the device is idle, we need to start a transmission. Otherwise,
  // the transmission will be started when the current packet finished
  // transmission (see TransmitCompleteEvent)
  //
  if (m_txMachineState == READY)
    {
      if (m_queue->IsEmpty () == false)
        {
          Ptr<Packet> packet = m_queue->Dequeue ();
          NS_ASSERT_MSG (packet != 0,
                         "CustomP2PNetDevice::SendFrom(): IsEmpty false but no packet on queue?");
          m_currentPkt = packet;
          m_promiscSnifferTrace (m_currentPkt);
          m_snifferTrace (m_currentPkt);
          TransmitStart (m_currentPkt);
        }
    }
  return true;
}

Ptr<Node>
CustomP2PNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void
CustomP2PNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
CustomP2PNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
CustomP2PNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

void
CustomP2PNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscCallback = cb;
}

bool
CustomP2PNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
CustomP2PNetDevice::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

Address
CustomP2PNetDevice::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (std::size_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

bool
CustomP2PNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
CustomP2PNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

// uint16_t
// CustomP2PNetDevice::PppToEther (uint16_t proto)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   switch (proto)
//     {
//     case 0x0021:
//       return 0x0800; //IPv4
//     case 0x0057:
//       return 0x86DD; //IPv6
//     default:
//       NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
//     }
//   return 0;
// }

// uint16_t
// CustomP2PNetDevice::EtherToPpp (uint16_t proto)
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   switch (proto)
//     {
//     case 0x0800:
//       return 0x0021; //IPv4
//     case 0x86DD:
//       return 0x0057; //IPv6
//     default:
//       NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
//     }
//   return 0;
// }

// bool
// CustomP2PNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocol)
// {
//   NS_LOG_FUNCTION (this << packet << dest << protocol);

//   // Add custom header before sending
//   AddCustomHeader (packet, dest, protocol);

//   // Call the base class or the actual transmission method (if needed)
//   // Example: Forward to the lower layers or physical medium
//   NS_LOG_INFO ("Sending packet with custom header.");
//   return true; // Assume successful transmission
// }

void
CustomP2PNetDevice::SetWithCustomHeader (bool withHeader)
{
  m_withCustomHeader = withHeader;
}

bool
CustomP2PNetDevice::IsWithCustomHeader (void) const
{
  return m_withCustomHeader;
}

void
CustomP2PNetDevice::AddCustomHeader (Ptr<Packet> packet, const Address &dest, uint16_t protocol)
{
  NS_LOG_FUNCTION (this << packet << dest << protocol);

  // Add Ethernet header
  EthernetHeader ethHeader;
  ethHeader.SetSource (Mac48Address ("00:11:22:33:44:55")); // Example source MAC
  ethHeader.SetDestination (Mac48Address::ConvertFrom (dest));
  ethHeader.SetLengthType (protocol);
  packet->AddHeader (ethHeader);

  //   // Add custom P4 header
  //   CustomHeader customHeader ();
  //   customHeader.SetLayer (LAYER_5);

  //   customHeader.AddField ("proto_id", 16);
  //   customHeader.AddField ("dst_id", 16);
  //   customHeader.SetField ("proto_id", protocol);
  //   customHeader.SetField ("dst_id", 42); // Example custom field
  //   packet->AddHeader (customHeader);

  NS_LOG_INFO ("Added custom header to packet.");
}

} // namespace ns3
