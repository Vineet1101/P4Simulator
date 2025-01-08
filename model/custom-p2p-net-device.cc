/**
 * 
 * Header : [Layer 2] [Layer 3] [Layer 4]
 *          [EthernetHeader] [UserDefined] [Ipv4Header] [UserDefined] [UdpHeader] [UserDefined]
 * 
 * 使用不同的端口进行区分：是否需要加入custiom header
 * 
 */

#include "custom-p2p-net-device.h"

#include "ns3/log.h"

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include <ns3/global.h>
#include <stack>

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
    : m_txMachineState (READY), m_channel (0), m_protocol (0x1), m_linkUp (false), m_currentPkt (0)
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
CustomP2PNetDevice::AddHeader (Ptr<Packet> p, Mac48Address source, Mac48Address dest,
                               uint16_t protocolNumber)
{
  /*
      *** Ethernet protocolNumber: 0x800
    ==========*** Print Packet Headers ***==========
    ns3::EthernetHeader ( length/type=0x12, source=00:00:00:00:00:01, destination=ff:ff:ff:ff:ff:ff) ns3::CustomHeader (CustomHeader { }) ns3::Ipv4Header (tos 0x0 DSCP Default ECN Not-ECT ttl 64 id 0 protocol 17 offset (bytes) 0 flags [none] length: 1028 10.1.1.1 > 10.1.1.2) ns3::UdpHeader (length: 1008 49153 > 9093) Payload (size=1000) 
    * Ethernet header detecting, packet length: 1050
    ** Ethernet packet
    Network order: 0x12
    Host order: 0x1200
    *** Ethernet header: Source MAC: 00:00:00:00:00:01, Destination MAC: ff:ff:ff:ff:ff:ff, Protocol: 0x12
    Parser: Ethernet protocol: 18
    * Custom header detecting, packet length: 1036
    ** Custom header detected
    *** Custom header content: 
    CustomHeader { Field1: 0x800 Field2: 0x1234 Field3: 0x89ABCDEF }
    * IPv4 header detecting, packet length: 1028
    ** IPv4 packet
    *** IPv4 header: Source IP: 10.1.1.1, Destination IP: 10.1.1.2, TTL: 64, Protocol: 17
    Parser: IPv4 protocol: 17
    * UDP header detecting, packet length: 1008
    ** UDP packet
    *** UDP header: Source Port: 49153, Destination Port: 9093
    ==========*** ====== ***==========
  */
  NS_LOG_FUNCTION (p << source << dest << protocolNumber);

  // ADD the Ethernet header with "new" MAC address
  EthernetHeader eeh_header (false);
  eeh_header.SetSource (source);
  eeh_header.SetDestination (dest);

  NS_LOG_DEBUG ("Ethernet protocolNumber: " << std::hex << "0x" << protocolNumber << std::dec);
  eeh_header.SetLengthType (protocolNumber);
  // uint16_t ttt = eeh_header.GetLengthType ();
  // NS_LOG_DEBUG ("*** Ethernet protocolNumber: " << std::hex << "0x" << ttt << std::dec);

  NS_LOG_DEBUG ("Sending: before adding the ethernet header and custom header");
  p->Print (std::cout);
  std::cout << " " << std::endl;

  // Check if should add the custom header
  Ipv4Header ip_hd;
  uint16_t dst_port = 0;
  bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
  if (hasIpv4)
    {
      // Set the ipv4'protocol number to the custom header
      uint16_t protocol_temp = ip_hd.GetProtocol ();
      m_header.SetProtocolFieldNumber (0);

      if (protocol_temp == 0x11)
        {
          NS_LOG_DEBUG ("UDP protocol, get the port number");
          UdpHeader udp_hd;
          p->PeekHeader (udp_hd);
          dst_port = udp_hd.GetDestinationPort ();
        }
      else if (protocol_temp == 0x06)
        {
          NS_LOG_DEBUG ("TCP protocol, get the port number");
          TcpHeader tcp_hd;
          p->PeekHeader (tcp_hd);
          dst_port = tcp_hd.GetDestinationPort ();
        }
      else
        {
          NS_LOG_WARN ("Unknown protocol number, unable to get the port number");
        }
      p->AddHeader (ip_hd);
    }
  else
    {
      NS_LOG_WARN ("No IPv4 header found in the packet, so we can't add the custom header");
    }

  if (!dst_port || (dst_port < P4GlobalVar::g_portRangeMin) ||
      (dst_port > P4GlobalVar::g_portRangeMax))
    {

      NS_LOG_DEBUG ("Sending: after adding the header");
      p->EnablePrinting ();
      p->Print (std::cout);
      std::cout << " " << std::endl;

      // not in the range, so we don't add the custom header
      NS_LOG_DEBUG ("Checked the udp/tcp port number " << dst_port
                                                       << ", no need to add the custom header.");
      p->AddHeader (eeh_header);
      return;
    }

  if (m_header.GetLayer () == HeaderLayer::LAYER_2)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // In this case, we set m_protocol (parser start number)
          // Layer 2: [custom] [ethernet] layer 3: [ipv4] layer 4: [udp/tcp]
          m_protocol = P4GlobalVar::g_p4Protocol;
          m_header.SetProtocolFieldNumber (0x1); // ethernet

          p->AddHeader (eeh_header);
          p->AddHeader (m_header);
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
        {
          // In this case, we set m_protocol (parser start number)
          // Layer 2: [custom] layer 3: [ipv4] layer 4: [udp/tcp]
          m_protocol = P4GlobalVar::g_p4Protocol;
          m_header.SetProtocolFieldNumber (0x0800); //ipv4

          p->AddHeader (m_header);
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
        {
          // Layer 2: [ethernet] [custom] layer 3: [ipv4] layer 4: [udp/tcp]
          eeh_header.SetLengthType (P4GlobalVar::g_p4Protocol);
          m_header.SetProtocolFieldNumber (0x0800); //ipv4

          p->AddHeader (m_header);
          p->AddHeader (eeh_header);
        }
      else
        {
          NS_LOG_WARN ("Unknown operator for LAYER_2");
        }
    }
  else if (m_header.GetLayer () == HeaderLayer::LAYER_3)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // Layer 2: [ethernet] layer 3: [custom] [ipv4] layer 4: [udp/tcp]
          eeh_header.SetLengthType (P4GlobalVar::g_p4Protocol);
          m_header.SetProtocolFieldNumber (0x0800); //ipv4

          p->AddHeader (m_header);
          p->AddHeader (eeh_header);
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
        {
          // Layer 2: [ethernet] layer 3: [custom] layer 4: [udp/tcp]
          eeh_header.SetLengthType (P4GlobalVar::g_p4Protocol);

          Ipv4Header ip_hd;
          bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
          if (hasIpv4)
            {
              uint16_t protocol_temp = ip_hd.GetProtocol ();
              m_header.SetProtocolFieldNumber (protocol_temp);

              p->AddHeader (m_header);
              p->AddHeader (eeh_header);
            }
          else
            {
              NS_LOG_WARN ("No IPv4 header found in the packet");
            }
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
        {
          // Layer 2: [ethernet] layer 3: [ipv4] [custom] layer 4: [udp/tcp]
          Ipv4Header ip_hd;
          bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
          if (hasIpv4)
            {
              ip_hd.SetProtocol (P4GlobalVar::g_p4Protocol);
              uint16_t protocol_temp = ip_hd.GetProtocol ();
              m_header.SetProtocolFieldNumber (protocol_temp);

              p->AddHeader (m_header);
              p->AddHeader (ip_hd);
              p->AddHeader (eeh_header);
            }
          else
            {
              NS_LOG_WARN ("No IPv4 header found in the packet");
            }
        }
    }
  else if (m_header.GetLayer () == HeaderLayer::LAYER_4)
    {
      if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
        {
          // Layer 2: [ethernet] layer 3: [ipv4] [custom] layer 4: [udp/tcp]
          Ipv4Header ip_hd;
          bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
          if (hasIpv4)
            {
              ip_hd.SetProtocol (P4GlobalVar::g_p4Protocol);
              uint16_t protocol_temp = ip_hd.GetProtocol ();
              m_header.SetProtocolFieldNumber (protocol_temp);

              p->AddHeader (m_header);
              p->AddHeader (ip_hd);
              p->AddHeader (eeh_header);
            }
          else
            {
              NS_LOG_WARN ("No IPv4 header found in the packet");
            }
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
        {
          // Layer 2: [ethernet] layer 3: [ipv4] layer 4: [custom]
          Ipv4Header ip_hd;
          bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
          if (hasIpv4)
            {
              // Set the ipv4'protocol number to the custom header
              uint16_t protocol_temp = ip_hd.GetProtocol ();
              if (protocol_temp == 0x11)
                {
                  NS_LOG_DEBUG ("Custom header replace UDP protocol");
                  UdpHeader udp_hd;
                  p->RemoveHeader (udp_hd); // Remove UDP header
                }
              else if (protocol_temp == 0x06)
                {
                  NS_LOG_DEBUG ("Custom header replace TCP protocol");
                  TcpHeader tcp_hd;
                  p->RemoveHeader (tcp_hd); // Remove TCP header
                }
              else
                {
                  NS_LOG_INFO ("No Layer 4 protocol found in the packet");
                }

              // Set the ipv4'protocol number to the custom header
              ip_hd.SetProtocol (P4GlobalVar::g_p4Protocol);
              m_header.SetProtocolFieldNumber (0);
              p->AddHeader (m_header);
              p->AddHeader (ip_hd);
              p->AddHeader (eeh_header);
            }
          else
            {
              NS_LOG_WARN ("LAYER 4 REPLACE: No IPv4 header found in the packet");
            }
        }
      else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
        {
          // Layer 2: [ethernet] layer 3: [ipv4] layer 4: [udp/tcp] [custom]
          Ipv4Header ip_hd;
          bool hasIpv4 = p->RemoveHeader (ip_hd); // Remove IPv4 header
          if (hasIpv4)
            {
              // Set the ipv4'protocol number to the custom header
              uint16_t protocol_temp = ip_hd.GetProtocol ();
              m_header.SetProtocolFieldNumber (0);

              if (protocol_temp == 0x11)
                {
                  NS_LOG_DEBUG ("Custom header replace UDP protocol");
                  UdpHeader udp_hd;
                  p->RemoveHeader (udp_hd); // Remove UDP header
                  p->AddHeader (m_header);
                  p->AddHeader (udp_hd);
                }
              else if (protocol_temp == 0x06)
                {
                  NS_LOG_DEBUG ("Custom header replace TCP protocol");
                  TcpHeader tcp_hd;
                  p->RemoveHeader (tcp_hd); // Remove TCP header
                  p->AddHeader (m_header);
                  p->AddHeader (tcp_hd);
                }
              else
                {
                  NS_LOG_WARN ("LAYER 4 ADD_AFTER: Unknown protocol number");
                }
              p->AddHeader (ip_hd);
              p->AddHeader (eeh_header);
            }
        }

      NS_LOG_DEBUG ("Ethernet header added, packet size: " << p->GetSize ());
    }
  else
    {
      NS_LOG_WARN ("Unknown layer for the custom header");
    }

  NS_LOG_DEBUG ("Sending: after adding the header");
  p->EnablePrinting ();
  p->Print (std::cout);
  std::cout << " " << std::endl;
}

bool
CustomP2PNetDevice::ProcessHeader (Ptr<Packet> p, uint16_t &param)
{
  NS_LOG_FUNCTION (this << p << param);

  if (m_withCustomHeader)
    {
      // accelerate, if not, no need for checking that.
      RestoreOriginalHeaders (p);
    }
  return true;
}

void
CustomP2PNetDevice::RestoreOriginalHeaders (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  // Restore the original headers
  // for the switch port net-device, no need to processing the header.

  EthernetHeader eeh_hd;
  Ipv4Header ip_hd;
  ArpHeader arp_hd;
  UdpHeader udp_hd;
  TcpHeader tcp_hd;

  uint64_t protocol = CheckIfEthernetHeader (p, eeh_hd);
  std::stack<uint64_t> protocolStack; // Storage protocol stack path

  while (protocol != 0)
    {
      protocolStack.push (protocol);

      switch (protocol)
        {
        case 0x1: // Ethernet (Assume in p4sim)
          NS_LOG_DEBUG ("Parser: Ethernet protocol: " << protocol);
          break;
        case 0x0800: // IPv4
          protocol = CheckIfIpv4Header (p, ip_hd);
          NS_LOG_DEBUG ("Parser: IPv4 protocol: " << (uint32_t) protocol);
          break;
        case 0x0806: // ARP
          protocol = CheckIfArpHeader (p, arp_hd);
          break;
        case 0x86DD: // IPv6
          protocol = 0;
          break;
        case 0x11: // UDP (0x11 == 17)
          protocol = CheckIfUdpHeader (p, udp_hd);
          if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
              m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
            {
              protocol = P4GlobalVar::g_p4Protocol;
            }
          break;
        case 0x06: // TCP
          protocol = CheckIfTcpHeader (p, tcp_hd);
          if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
              m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
            {
              protocol = P4GlobalVar::g_p4Protocol;
            }
          break;
        case P4GlobalVar::g_p4Protocol: // Custom Protocol
          protocol = CheckIfCustomHeader (p);
          break;
        default:
          protocol = 0; // End
          break;
        }
    }

  // **Reverse traversal**
  NS_LOG_DEBUG ("Start reverse parsing");
  while (!protocolStack.empty ())
    {
      uint64_t reverseProtocol = protocolStack.top ();
      protocolStack.pop ();

      switch (reverseProtocol)
        {
        case P4GlobalVar::g_p4Protocol:
          NS_LOG_DEBUG ("Reverse Parser: Custom P4 Header");
          break;
        case 0x11:
          NS_LOG_DEBUG ("Reverse Parser: UDP Header");
          p->AddHeader (udp_hd);
          break;
        case 0x06:
          NS_LOG_DEBUG ("Reverse Parser: TCP Header");
          p->AddHeader (tcp_hd);
          break;
        case 0x0800:
          NS_LOG_DEBUG ("Reverse Parser: IPv4 Header");
          p->AddHeader (ip_hd);
          break;
        case 0x1:
          NS_LOG_DEBUG ("Reverse Parser: Ethernet Header");
          p->AddHeader (eeh_hd);
          break;
        default:
          NS_LOG_DEBUG ("Reverse Parser: Unknown Header");
          break;
        }
    }

  // // Parser with the order
  // if (m_header.GetLayer () == HeaderLayer::LAYER_2)
  //   {
  //     if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
  //       {
  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
  //       {
  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         // Add Ethernet header @todo
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         p->AddHeader (eeh);
  //       }
  //     else
  //       {
  //         NS_LOG_WARN ("No Operator for custom header!");
  //       }
  //   }
  // else if (m_header.GetLayer () == HeaderLayer::LAYER_3)
  //   {
  //     if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         p->AddHeader (eeh);
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         // Add Ethernet header @todo
  //         p->AddHeader (eeh);
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         Ipv4Header ip_hd;
  //         p->RemoveHeader (ip_hd);

  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         p->AddHeader (ip_hd);
  //         p->AddHeader (eeh);
  //       }
  //     else
  //       {
  //         NS_LOG_WARN ("No Operator for custom header!");
  //       }
  //   }
  // else if (m_header.GetLayer () == HeaderLayer::LAYER_4)
  //   {
  //     if (m_header.GetOperator () == HeaderLayerOperator::ADD_BEFORE)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         Ipv4Header ip_hd;
  //         p->RemoveHeader (ip_hd);

  //         bool hasCustom = p->PeekHeader (m_header);
  //         // with CallBack @todo
  //         if (hasCustom)
  //           {
  //             p->RemoveHeader (m_header);
  //           }
  //         p->AddHeader (ip_hd);
  //         p->AddHeader (eeh);
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::REPLACE)
  //       {
  //         NS_LOG_WARN ("Not support yet now");
  //       }
  //     else if (m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
  //       {
  //         EthernetHeader eeh;
  //         p->RemoveHeader (eeh);
  //         Ipv4Header ip_hd;
  //         bool hasIpv4 = p->RemoveHeader (ip_hd);
  //         if (hasIpv4)
  //           {
  //             // Set the ipv4'protocol number to the custom header
  //             uint16_t protocol_temp = ip_hd.GetProtocol ();
  //             m_header.SetProtocolFieldNumber (0);

  //             if (protocol_temp == 0x11)
  //               {
  //                 NS_LOG_DEBUG ("Custom header replace UDP protocol");
  //                 UdpHeader udp_hd;
  //                 p->RemoveHeader (udp_hd); // Remove UDP header
  //                 p->RemoveHeader (m_header);
  //                 p->AddHeader (udp_hd);
  //               }
  //             else if (protocol_temp == 0x06)
  //               {
  //                 NS_LOG_DEBUG ("Custom header replace TCP protocol");
  //                 TcpHeader tcp_hd;
  //                 p->RemoveHeader (tcp_hd); // Remove TCP header
  //                 p->RemoveHeader (m_header);
  //                 p->AddHeader (tcp_hd);
  //               }
  //             else
  //               {
  //                 NS_LOG_WARN ("LAYER 4 ADD_AFTER: Unknown protocol number");
  //               }
  //             p->AddHeader (ip_hd);
  //             p->AddHeader (eeh);
  //           }
  //       }
  //     else
  //       {
  //         NS_LOG_WARN ("No Operator for custom header!");
  //       }
  //   }

  p->Print (std::cout);
  std::cout << " " << std::endl;

  // PrintPacketHeaders (p);
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

  NS_LOG_LOGIC ("Receiver SIDE Start: ");
  // PrintPacketHeaders (packet);

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

      // PrintPacketHeaders (packet); // @TEST

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
  /**
    ==========*** Print Packet Headers ***==========
    * Ethernet header detecting, packet length: 1049
    ** Ethernet packet
    *** Ethernet header: Source MAC: 00:00:00:00:00:01, Destination MAC: ff:ff:ff:ff:ff:ff, Protocol: 0x12
    * Custom header detecting, packet length: 1035
    ** Custom header detected
    *** Custom header content: 
    CustomHeader { Field1: 0x11 Field2: 0x1234 Field3: 0x89ABCDEF }
    * UDP header detecting, packet length: 1028
    ** UDP packet
    *** UDP header: Source Port: 17664, Destination Port: 1028
    ==========*** ====== ***==========
   */
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

uint16_t
CustomP2PNetDevice::CheckIfEthernetHeader (Ptr<Packet> p, EthernetHeader &eeh_hd)
{
  NS_LOG_DEBUG ("* Ethernet header detecting, packet length: " << p->GetSize ());
  // EthernetHeader eeh_hd;
  if (p->PeekHeader (eeh_hd))
    {
      NS_LOG_DEBUG ("** Ethernet packet");
      Mac48Address src_mac = eeh_hd.GetSource ();
      Mac48Address dst_mac = eeh_hd.GetDestination ();
      uint16_t protocol_eth = eeh_hd.GetLengthType (); // Network Byte Order

      NS_LOG_DEBUG ("*** Ethernet header: Source MAC: " << src_mac << ", Destination MAC: "
                                                        << dst_mac << ", Protocol: " << std::hex
                                                        << "0x" << protocol_eth << std::dec);
      p->RemoveHeader (eeh_hd);
      return protocol_eth;
    }
  else
    {
      NS_LOG_DEBUG ("No Ethernet header detected");
      return 0;
    }
}

uint8_t
CustomP2PNetDevice::CheckIfIpv4Header (Ptr<Packet> p, Ipv4Header &ip_hd)
{
  NS_LOG_DEBUG ("* IPv4 header detecting, packet length: " << p->GetSize ());

  // Ipv4Header ip_hd;
  if (p->PeekHeader (ip_hd))
    {
      NS_LOG_DEBUG ("** IPv4 packet");
      Ipv4Address src_ip = ip_hd.GetSource ();
      Ipv4Address dst_ip = ip_hd.GetDestination ();
      uint8_t ttl = ip_hd.GetTtl ();
      uint8_t protocol = ip_hd.GetProtocol ();
      NS_LOG_DEBUG ("*** IPv4 header: Source IP: " << src_ip << ", Destination IP: " << dst_ip
                                                   << ", TTL: " << (uint32_t) ttl
                                                   << ", Protocol: " << (uint32_t) protocol);
      p->RemoveHeader (ip_hd);
      return protocol;
    }
  else
    {
      NS_LOG_DEBUG ("** No IPv4 header detected");
      return 0;
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfArpHeader (Ptr<Packet> p, ArpHeader &arp_hd)
{
  // ARP will be the END, there is no option to modify the ARP header to add a followed custom header
  NS_LOG_DEBUG ("* ARP header detecting, packet length: " << p->GetSize ());
  // ArpHeader arp_hd;
  if (p->PeekHeader (arp_hd))
    {
      NS_LOG_DEBUG ("** ARP packet");
      Address src_mac = arp_hd.GetSourceHardwareAddress ();
      Address dst_mac = arp_hd.GetDestinationHardwareAddress ();
      Ipv4Address src_ip = arp_hd.GetSourceIpv4Address ();
      Ipv4Address dst_ip = arp_hd.GetDestinationIpv4Address ();
      NS_LOG_DEBUG ("*** ARP header: Source MAC: " << src_mac << ", Destination MAC: " << dst_mac
                                                   << ", Source IP: " << src_ip
                                                   << ", Destination IP: " << dst_ip);
      p->RemoveHeader (arp_hd);
    }
  else
    {
      NS_LOG_DEBUG ("** No ARP header detected");
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfCustomHeader (Ptr<Packet> p)
{
  // CustomHeader custom_hd;
  NS_LOG_DEBUG ("* Custom header detecting, packet length: " << p->GetSize ());

  // Here we must PeekHeader with the same FieldList as the CustomHeader,
  // otherwise, the PeekHeader will fail. So that we can't create a new
  // CustomHeader object to PeekHeader (NULL).
  if (p->PeekHeader (m_header))
    {
      NS_LOG_DEBUG ("** Custom header detected");
      NS_LOG_DEBUG ("*** Custom header content: ");
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
CustomP2PNetDevice::CheckIfUdpHeader (Ptr<Packet> p, UdpHeader &udp_hd)
{
  NS_LOG_DEBUG ("* UDP header detecting, packet length: " << p->GetSize ());
  // UdpHeader udp_hd;
  if (p->PeekHeader (udp_hd))
    {
      NS_LOG_DEBUG ("** UDP packet");
      uint16_t src_port = udp_hd.GetSourcePort ();
      uint16_t dst_port = udp_hd.GetDestinationPort ();
      NS_LOG_DEBUG ("*** UDP header: Source Port: " << src_port
                                                    << ", Destination Port: " << dst_port);
      p->RemoveHeader (udp_hd);
    }
  else
    {
      NS_LOG_DEBUG ("** No UDP header detected");
    }
  return 0;
}

uint64_t
CustomP2PNetDevice::CheckIfTcpHeader (Ptr<Packet> p, TcpHeader &tcp_hd)
{
  NS_LOG_DEBUG ("*TCP header detecting, packet length: " << p->GetSize ());
  // TcpHeader tcp_hd;
  if (p->PeekHeader (tcp_hd))
    {
      NS_LOG_DEBUG ("** TCP packet");
      // uint16_t src_port = tcp_hd.GetSourcePort ();
      // uint16_t dst_port = tcp_hd.GetDestinationPort ();
      NS_LOG_DEBUG ("*** TCP header: " << tcp_hd);
      p->RemoveHeader (tcp_hd);
    }
  else
    {
      NS_LOG_DEBUG ("** No TCP header detected");
    }
  return 0;
}

// void
// CustomP2PNetDevice::PrintPacketHeaders (Ptr<Packet> p)
// {
//   // Deprecated function

//   NS_LOG_DEBUG ("==========*** Print Packet Headers ***==========");
//   Ptr<Packet> copy = p->Copy ();

//   copy->EnablePrinting ();
//   copy->Print (std::cout);
//   std::cout << " " << std::endl;

//   if (!m_withCustomHeader)
//     {
//       uint16_t eth_protocol = CheckIfEthernetHeader (copy);
//       if (eth_protocol == 0x0800) // IPv4
//         {
//           uint8_t ipv4_protocol = CheckIfIpv4Header (copy);
//           if (ipv4_protocol == 17)
//             {
//               CheckIfUdpHeader (copy);
//             }
//           else if (ipv4_protocol == 6)
//             {
//               CheckIfTcpHeader (copy);
//             }
//         }
//       else if (eth_protocol == 0x0806) // ARP
//         {
//           CheckIfArpHeader (copy);
//         }
//     }
//   else
//     {
//       // @TODO
//       uint64_t protocol = m_protocol;
//       while (protocol != 0)
//         {
//           switch (protocol)
//             {
//             case 0x1: // Ethernet (Assume in p4sim)
//               protocol = CheckIfEthernetHeader (copy);
//               NS_LOG_DEBUG ("Parser: Ethernet protocol: " << protocol);
//               break;
//             case 0x0800: // IPv4
//               protocol = CheckIfIpv4Header (copy);
//               NS_LOG_DEBUG ("Parser: IPv4 protocol: " << (uint32_t) protocol);
//               break;
//             case 0x0806: // ARP
//               protocol = CheckIfArpHeader (copy);
//               break;
//             case 0x86DD: // IPv6
//               protocol = 0;
//               break;
//             case 0x11: // UDP (0x11 == 17)
//               protocol = CheckIfUdpHeader (copy);
//               if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
//                   m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
//                 {
//                   protocol = P4GlobalVar::g_p4Protocol;
//                 }
//               break;
//             case 0x06: // TCP
//               protocol = CheckIfTcpHeader (copy);
//               if (m_header.GetLayer () == HeaderLayer::LAYER_4 &&
//                   m_header.GetOperator () == HeaderLayerOperator::ADD_AFTER)
//                 {
//                   protocol = P4GlobalVar::g_p4Protocol;
//                 }
//               break;
//             case P4GlobalVar::g_p4Protocol: // Custom Protocol
//               protocol = CheckIfCustomHeader (copy);
//               break;
//             default:
//               protocol = 0; // End
//               break;
//             }
//         }
//     }

//   NS_LOG_DEBUG ("==========*** ====== ***==========");
// }

} // namespace ns3
