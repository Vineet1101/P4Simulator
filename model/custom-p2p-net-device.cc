/**
 * 
 * Header : [Layer 2] [Layer 3] [Layer 4]
 *          [EthernetHeader] [UserDefined] [Ipv4Header] [UserDefined] [UdpHeader] [UserDefined]
 * 
 */

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

#include <ns3/arp-l3-protocol.h>
#include <ns3/arp-header.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/tcp-header.h>
#include <ns3/udp-header.h>

#include <ns3/global.h>
// #include "ns3/ppp-header.h"

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
    : m_txMachineState (READY), m_channel (0), m_protocol (1), m_linkUp (false), m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
  // m_protocol = 0x1; // Ethernet protocol number (start parser with Ethernet)
}

CustomP2PNetDevice::~CustomP2PNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
CustomP2PNetDevice::SetCustomHeader (CustomHeader customHeader)
{
  m_header = customHeader;
  m_withCustomHeader = true;
}

void
CustomP2PNetDevice::AddCustomHeader (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_DEBUG ("packet with length " << p->GetSize ());
  if (m_withCustomHeader)
    {
      // Insert / Add the custom header to the packet for the host
      EthernetHeader eeh;
      bool hasEthernet = p->RemoveHeader (eeh); // Peek Ethernet header
      NS_LOG_DEBUG ("Ethernet header found: " << hasEthernet
                                              << " packet size (after remove): " << p->GetSize ());

      Ipv4Header ip_hd;
      bool hasIpv4 = p->RemoveHeader (ip_hd); // Peek IPv4 header
      NS_LOG_DEBUG ("IPv4 header found: " << hasIpv4
                                          << " packet size (after remove): " << p->GetSize ());

      ArpHeader arp_hd;
      bool hasArp = p->RemoveHeader (arp_hd);
      NS_LOG_DEBUG ("ARP header found: " << hasArp
                                         << " packet size (after remove): " << p->GetSize ());

      HeaderLayer layer = m_header.GetLayer ();
      HeaderLayerOperator op = m_header.GetOperator ();

      // // Remove existing headers
      // if (hasEthernet)
      //   {
      //     p->RemoveHeader (eeh);
      //   }
      // if (hasIpv4)
      //   {
      //     p->RemoveHeader (ip_hd);
      //   }
      // if (hasArp)
      //   {
      //     p->RemoveHeader (arp_hd);
      //   }

      // Process based on layer and operation
      if (layer == HeaderLayer::LAYER_3) // Network Layer (IPv4/ARP)
        {
          if (op == HeaderLayerOperator::ADD_BEFORE) // Add CustomHeader before existing L3 headers
            {
              if (hasIpv4)
                {
                  p->AddHeader (ip_hd);
                }
              else if (hasArp)
                {
                  p->AddHeader (arp_hd);
                }
              p->AddHeader (m_header);
            }
          else if (op == HeaderLayerOperator::REPLACE) // Replace L3 header with CustomHeader
            {
              p->AddHeader (m_header);
            }
          else if (op == HeaderLayerOperator::ADD_AFTER) // Add CustomHeader after L3 headers
            {
              p->AddHeader (m_header);
              if (hasIpv4)
                {
                  p->AddHeader (ip_hd);
                }
              else if (hasArp)
                {
                  p->AddHeader (arp_hd);
                }
            }
          // Add the under layer
          p->AddHeader (eeh);
        }
      else if (layer == LAYER_2) // Data Link Layer (Ethernet)
        {
          if (op == HeaderLayerOperator::ADD_BEFORE) // Add CustomHeader before Ethernet
            {
              if (hasEthernet)
                {
                  p->AddHeader (eeh);
                }
              p->AddHeader (m_header);
            }
          else if (op == HeaderLayerOperator::REPLACE) // Replace Ethernet header
            {
              p->AddHeader (m_header);
            }
          else if (op == HeaderLayerOperator::ADD_AFTER) // Add CustomHeader after Ethernet
            {
              p->AddHeader (m_header);
              if (hasEthernet)
                {
                  p->AddHeader (eeh);
                }
            }
        }
      // No layer under Ethernet
    }

  // 检查 CustomHeader是否正确添加
  PrintPacketHeaders (p);
}

// void
// CustomP2PNetDevice::AddHeader (Ptr<Packet> p, uint16_t protocolNumber)
// {
//   NS_LOG_FUNCTION (this << p << protocolNumber);

//   // add the header @TODO

//   // EthernetHeader header
//   EthernetHeader header (false);
//   bool hasEthernet = p->PeekHeader (header); // Peek Ethernet header
//   if (hasEthernet)
//     {
//       // Remove the existing Ethernet header
//       p->RemoveHeader (header);
//     }
//   header.SetSource (source);
//   header.SetDestination (dest);

//   // EthernetTrailer trailer;

//   p->AddHeader (header);

//   AddCustomHeader (p);
// }

void
CustomP2PNetDevice::AddHeader (Ptr<Packet> p, Mac48Address source, Mac48Address dest,
                               uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (p << source << dest << protocolNumber);

  // ADD the Ethernet header with "new" MAC address
  EthernetHeader eeh_header (false);
  eeh_header.SetSource (source);
  eeh_header.SetDestination (dest);
  eeh_header.SetLengthType (0x800); // IPv4

  // Set the protocol number in the header
  if (m_header.GetLayer () == HeaderLayer::LAYER_2)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::REPLACE ||
          m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // In this case, we set m_protocol (parser start number)
          m_protocol = P4GlobalVar::g_p4Protocol;
          // and other header no change
        }
    }
  else if (m_header.GetLayer () == HeaderLayer::LAYER_3)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::REPLACE ||
          m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // In this case, we set the protocol number in Ethernet header to the custom header
          eeh_header.SetLengthType (P4GlobalVar::g_p4Protocol);
        }
      else
        {
          // In this ADD_AFTER case, we set the ipv4'protocol number to the custom header
          if (protocolNumber == 0x800)
            {
              Ipv4Header ip_hd;
              bool hasIpv4 = p->RemoveHeader (ip_hd); // Peek IPv4 header
              if (hasIpv4)
                {
                  ip_hd.SetProtocol (P4GlobalVar::g_p4Protocol);
                }
              p->AddHeader (ip_hd);
            }
          // and other header no change
        }
    }
  else if (m_header.GetLayer () == HeaderLayer::LAYER_4)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::REPLACE ||
          m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // Modify the ipv4 header protocol number to the custom header
          if (protocolNumber == 0x800)
            {
              Ipv4Header ip_hd;
              bool hasIpv4 = p->RemoveHeader (ip_hd); // Peek IPv4 header
              if (hasIpv4)
                {
                  ip_hd.SetProtocol (P4GlobalVar::g_p4Protocol);
                }
              p->AddHeader (ip_hd);
            }
        }
      else
        {
          // In this ADD_AFTER case, because the udp don't have the protocol number,
          // so we don't need to change the protocol number
          NS_LOG_DEBUG ("LAYER 4 ADD AFTER: No need to change the protocol number");
        }
    }
  p->AddHeader (eeh_header);
  NS_LOG_DEBUG ("Ethernet header added, packet size: " << p->GetSize ());

  // if (Node::ChecksumEnabled ())
  //   {
  //     trailer.EnableFcs (true);
  //   }
  // trailer.CalcFcs (p);
  // p->AddTrailer (trailer);

  AddCustomHeader (p);
}

bool
CustomP2PNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t &param)
{
  NS_LOG_FUNCTION (this << p << param);
  // // process the header @TODO
  // PppHeader ppp;
  // p->RemoveHeader (ppp);
  // param = PppToEther (ppp.GetProtocol ());

  if (m_withCustomHeader)
    {
      // change back to normal header for host
      RestoreOriginalHeaders (p);
    }
  return true;
}

void
CustomP2PNetDevice::RestoreOriginalHeaders (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  // Restore the original headers @TODO
  // for the switch port net-device, no need to processing the header.

  EthernetHeader eeh;
  bool hasEthernet = p->PeekHeader (eeh); // Peek Ethernet header

  Ipv4Header ip_hd;
  bool hasIpv4 = p->PeekHeader (ip_hd); // Peek IPv4 header

  ArpHeader arp_hd;
  bool hasArp = false;

  bool hasCustom = p->PeekHeader (m_header); // Peek CustomHeader

  if (!hasIpv4) // If no IPv4, check for ARP
    {
      hasArp = p->PeekHeader (arp_hd);
    }

  // **移除 Headers**
  if (hasEthernet)
    {
      p->RemoveHeader (eeh);
    }
  if (hasIpv4)
    {
      p->RemoveHeader (ip_hd);
    }
  if (hasArp)
    {
      p->RemoveHeader (arp_hd);
    }
  if (hasCustom)
    {
      p->RemoveHeader (m_header); // 移除 CustomHeader
    }

  // **恢复原始 Headers**
  if (hasEthernet)
    {
      p->AddHeader (eeh);
    }
  if (hasIpv4)
    {
      p->AddHeader (ip_hd);
    }
  if (hasArp)
    {
      p->AddHeader (arp_hd);
    }

  // 检查 CustomHeader是否正确移除
  PrintPacketHeaders (p);
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

  PrintPacketHeaders (packet);

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

      PrintPacketHeaders (packet); // @TEST

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
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
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

  AddHeader (packet, mac_source, mac_destination, protocolNumber);

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

// void
// CustomP2PNetDevice::AddCustomHeader (Ptr<Packet> packet, const Address &dest, uint16_t protocol)
// {
//   NS_LOG_FUNCTION (this << packet << dest << protocol);

//   // Add Ethernet header
//   EthernetHeader ethHeader;
//   ethHeader.SetSource (Mac48Address ("00:11:22:33:44:55")); // Example source MAC
//   ethHeader.SetDestination (Mac48Address::ConvertFrom (dest));
//   ethHeader.SetLengthType (protocol);
//   packet->AddHeader (ethHeader);

//   //   // Add custom P4 header
//   //   CustomHeader customHeader ();
//   //   customHeader.SetLayer (LAYER_5);

//   //   customHeader.AddField ("proto_id", 16);
//   //   customHeader.AddField ("dst_id", 16);
//   //   customHeader.SetField ("proto_id", protocol);
//   //   customHeader.SetField ("dst_id", 42); // Example custom field
//   //   packet->AddHeader (customHeader);

//   NS_LOG_INFO ("Added custom header to packet.");
// }

uint16_t
CustomP2PNetDevice::CheckIfEthernetHeader (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("Ethernet header detecting, packet length: " << p->GetSize ());
  EthernetHeader eeh;
  if (p->PeekHeader (eeh))
    {
      NS_LOG_DEBUG ("Ethernet packet");
      Mac48Address src_mac = eeh.GetSource ();
      Mac48Address dst_mac = eeh.GetDestination ();
      uint16_t protocol_eth = eeh.GetLengthType ();
      NS_LOG_DEBUG ("* Ethernet header: Source MAC: " << src_mac << ", Destination MAC: " << dst_mac
                                                      << ", Protocol: " << std::hex << "0x"
                                                      << protocol_eth << std::dec);
      p->RemoveHeader (eeh);
      return protocol_eth;
    }
  else
    {
      NS_LOG_DEBUG ("No Ethernet header detected");
      return 0;
    }
}

uint8_t
CustomP2PNetDevice::CheckIfIpv4Header (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("IPv4 header detecting, packet length: " << p->GetSize ());

  Ipv4Header ip_hd;
  if (p->PeekHeader (ip_hd))
    {
      NS_LOG_DEBUG ("IPv4 packet");
      Ipv4Address src_ip = ip_hd.GetSource ();
      Ipv4Address dst_ip = ip_hd.GetDestination ();
      uint8_t ttl = ip_hd.GetTtl ();
      uint8_t protocol = ip_hd.GetProtocol ();
      NS_LOG_DEBUG ("* IPv4 header: Source IP: " << src_ip << ", Destination IP: " << dst_ip
                                                 << ", TTL: " << (uint32_t) ttl
                                                 << ", Protocol: " << (uint32_t) protocol);
      p->RemoveHeader (ip_hd);
      return protocol;
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfArpHeader (Ptr<Packet> p)
{
  // ARP will be the END, there is no option to modify the ARP header to add a followed custom header
  NS_LOG_DEBUG ("ARP header detecting, packet length: " << p->GetSize ());
  ArpHeader arp_hd;
  if (p->PeekHeader (arp_hd))
    {
      NS_LOG_DEBUG ("ARP packet");
      Address src_mac = arp_hd.GetSourceHardwareAddress ();
      Address dst_mac = arp_hd.GetDestinationHardwareAddress ();
      Ipv4Address src_ip = arp_hd.GetSourceIpv4Address ();
      Ipv4Address dst_ip = arp_hd.GetDestinationIpv4Address ();
      NS_LOG_DEBUG ("* ARP header: Source MAC: " << src_mac << ", Destination MAC: " << dst_mac
                                                 << ", Source IP: " << src_ip
                                                 << ", Destination IP: " << dst_ip);
      p->RemoveHeader (arp_hd);
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfCustomHeader (Ptr<Packet> p)
{
  // CustomHeader custom_hd;
  NS_LOG_DEBUG ("Custom header detecting, packet length: " << p->GetSize ());

  // Here we must PeekHeader with the same FieldList as the CustomHeader,
  // otherwise, the PeekHeader will fail. So that we can't create a new
  // CustomHeader object to PeekHeader (NULL).
  if (p->PeekHeader (m_header))
    {
      NS_LOG_DEBUG ("Custom header detected");
      NS_LOG_DEBUG ("** Custom header content: ");
      m_header.Print (std::cout);
      std::cout << std::endl;
      p->RemoveHeader (m_header);

      // Get the protocol number for next header
      if (m_header.GetProtocolNumber () != 0)
        {
          return m_header.GetProtocolNumber ();
        }
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfUdpHeader (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("UDP header detecting, packet length: " << p->GetSize ());
  UdpHeader udp_hd;
  if (p->PeekHeader (udp_hd))
    {
      NS_LOG_DEBUG ("UDP packet");
      uint16_t src_port = udp_hd.GetSourcePort ();
      uint16_t dst_port = udp_hd.GetDestinationPort ();
      NS_LOG_DEBUG ("* UDP header: Source Port: " << src_port
                                                  << ", Destination Port: " << dst_port);
      p->RemoveHeader (udp_hd);
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfTcpHeader (Ptr<Packet> p)
{
  NS_LOG_DEBUG ("TCP header detecting, packet length: " << p->GetSize ());
  TcpHeader tcp_hd;
  if (p->PeekHeader (tcp_hd))
    {
      // uint16_t src_port = tcp_hd.GetSourcePort ();
      // uint16_t dst_port = tcp_hd.GetDestinationPort ();
      NS_LOG_DEBUG ("* TCP header: " << tcp_hd);
      p->RemoveHeader (tcp_hd);
    }
  return 0;
}

void
CustomP2PNetDevice::PrintPacketHeaders (Ptr<Packet> p)
{
  Ptr<Packet> copy = p->Copy ();
  uint16_t eth_protocol = 0;
  uint8_t ipv4_protocol = 0;

  if (!m_withCustomHeader)
    {
      eth_protocol = CheckIfEthernetHeader (copy);
      if (eth_protocol == 0x0800) // IPv4
        {
          ipv4_protocol = CheckIfIpv4Header (copy);
          if (ipv4_protocol == 17)
            {
              CheckIfUdpHeader (copy);
            }
          else if (ipv4_protocol == 6)
            {
              CheckIfTcpHeader (copy);
            }
        }
      else if (eth_protocol == 0x0806) // ARP
        {
          CheckIfArpHeader (copy);
        }
    }
  else
    {
      // @TODO
      uint64_t protocol = m_protocol;
      while (protocol != 0)
        {
          switch (protocol)
            {
            case 0x1: // Ethernet (Assume in p4sim)
              protocol = CheckIfEthernetHeader (copy);
              break;
            case 0x0800: // IPv4
              protocol = CheckIfIpv4Header (copy);
              break;
            case 0x0806: // ARP
              protocol = CheckIfArpHeader (copy);
              break;
            case 0x86DD: // IPv6
              protocol = 0;
              break;
            case 0x11: // UDP (0x11 == 17)
              protocol = CheckIfUdpHeader (copy);
              if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
                  m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
                {
                  protocol = P4GlobalVar::g_p4Protocol;
                }
              break;
            case 0x06: // TCP
              protocol = CheckIfTcpHeader (copy);
              if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
                  m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
                {
                  protocol = P4GlobalVar::g_p4Protocol;
                }
              break;
            case P4GlobalVar::g_p4Protocol: // Custom Protocol
              protocol = CheckIfCustomHeader (copy);
              break;
            default:
              protocol = 0; // End
              break;
            }
        }
    }

  // // Step 1: 解析 CustomHeader（如果 ADD_BEFORE）
  // if (m_withCustomHeader && m_header.GetLayer () == LAYER_2 &&
  //     (m_header.GetOperator () == ADD_BEFORE || m_header.GetOperator () == REPLACE))
  //   {
  //     custom_defined = CheckIfCustomHeader (copy);
  //   }

  // if (m_withCustomHeader && m_header.GetLayer () == LAYER_2 &&
  //     (m_header.GetOperator () == ADD_BEFORE || m_header.GetOperator () == ADD_AFTER))
  //   {
  //     eth_protocol = CheckIfEthernetHeader (copy);
  //   }

  // if (m_withCustomHeader &&
  //         (m_header.GetLayer () == LAYER_2 && m_header.GetOperator () == ADD_AFTER) ||
  //     (m_header.GetLayer () == LAYER_3 &&
  //      (m_header.GetOperator () == ADD_BEFORE || m_header.GetOperator () == REPLACE)))
  //   {
  //     custom_defined = CheckIfCustomHeader (copy);
  //   }

  // if (m_withCustomHeader && m_header.GetLayer () == LAYER_3 &&
  //     (m_header.GetOperator () == ADD_BEFORE || m_header.GetOperator () == ADD_AFTER))
  //   {
  //     if (eth_protocol == 0x0800) // IPv4
  //       {
  //         ipv4_protocol = CheckIfIpv4Header (copy);
  //         if (ipv4_protocol == 17)
  //           {
  //             CheckIfUdpHeader (copy);
  //           }
  //         else if (ipv4_protocol == 6)
  //           {
  //             CheckIfTcpHeader (copy);
  //           }
  //       }
  //     else if (eth_protocol == 0x0806) // ARP
  //       {
  //         CheckIfArpHeader (copy);
  //       }
  //   }

  // if (m_withCustomHeader && m_header.GetLayer () == LAYER_4 &&
  //     (m_header.GetOperator () == ADD_BEFORE || m_header.GetOperator () == ADD_AFTER))
  //   {
  //     custom_defined = CheckIfCustomHeader (copy);
  //   }

  // // Step 5: 解析 CustomHeader（如果 ADD_AFTER）
  // if (m_withCustomHeader)
  //   {
  //     custom_defined = CheckIfCustomHeader (copy);
  //   }
}

} // namespace ns3
