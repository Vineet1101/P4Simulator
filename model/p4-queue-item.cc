/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Implementation of P4QueueItem
 */

#include "p4-queue-item.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

#include <bm/bm_sim/phv.h>

#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4QueueItem");

std::ostream &
operator<< (std::ostream &os, PacketType type)
{
  switch (type)
    {
    case PacketType::NORMAL:
      os << "NORMAL";
      break;
    case PacketType::RESUBMIT:
      os << "RESUBMIT";
      break;
    case PacketType::RECIRCULATE:
      os << "RECIRCULATE";
      break;
    case PacketType::SENTINEL:
      os << "SENTINEL";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  return os;
}

void
StandardMetadata::PrintMetadata (std::ostream &os) const
{
  os << "Standard Metadata Information:\n";
  os << "  " << std::setw (15) << "Ingress Port" << ": " << std::setw (10) << ingress_port << "\n"
     << "  " << std::setw (15) << "Egress Spec" << ": " << std::setw (10) << egress_spec << "\n"
     << "  " << std::setw (15) << "Egress Port" << ": " << std::setw (10) << egress_port << "\n"
     << "  " << std::setw (15) << "Instance Type" << ": " << std::setw (10) << instance_type << "\n"
     << "  " << std::setw (15) << "Packet Length" << ": " << std::setw (10) << packet_length << "\n"
     << "  " << std::setw (15) << "Enq Timestamp" << ": " << std::setw (10) << enq_timestamp << "\n"
     << "  " << std::setw (15) << "Enq Qdepth" << ": " << std::setw (10) << enq_qdepth << "\n"
     << "  " << std::setw (15) << "Deq Timedelta" << ": " << std::setw (10) << deq_timedelta << "\n"
     << "  " << std::setw (15) << "Deq Qdepth" << ": " << std::setw (10) << deq_qdepth << "\n"
     << "  " << std::setw (15) << "Ingress TS" << ": " << std::setw (10) << ingress_global_timestamp
     << "\n"
     << "  " << std::setw (15) << "Egress TS" << ": " << std::setw (10) << egress_global_timestamp
     << "\n"
     << "  " << std::setw (15) << "Mcast Group" << ": " << std::setw (10) << mcast_grp << "\n"
     << "  " << std::setw (15) << "Egress RID" << ": " << std::setw (10) << egress_rid << "\n"
     << "  " << std::setw (15) << "Checksum Err" << ": " << std::setw (10) << (int) checksum_error
     << "\n"
     << "  " << std::setw (15) << "Priority" << ": " << std::setw (10) << (int) priority << "\n"
     << "  " << std::setw (15) << "Parser Error" << ": " << std::setw (10) << parser_error
     << "\n\n";

  os.flush ();
}

void
StandardMetadata::GetMetadataFromBMPacket (std::unique_ptr<bm::Packet> &&bm_packet)
{
  bm::PHV *phv = bm_packet->get_phv ();

  ingress_port = phv->has_field ("standard_metadata.ingress_port")
                     ? phv->get_field ("standard_metadata.ingress_port").get_uint ()
                     : 0u;

  egress_spec = phv->has_field ("standard_metadata.egress_spec")
                    ? phv->get_field ("standard_metadata.egress_spec").get_uint ()
                    : 0u;

  egress_port = phv->has_field ("standard_metadata.egress_port")
                    ? phv->get_field ("standard_metadata.egress_port").get_uint ()
                    : 0u;

  instance_type = phv->has_field ("standard_metadata.instance_type")
                      ? phv->get_field ("standard_metadata.instance_type").get_uint ()
                      : 0u;

  packet_length = phv->has_field ("standard_metadata.packet_length")
                      ? phv->get_field ("standard_metadata.packet_length").get_uint ()
                      : 0u;

  enq_timestamp = phv->has_field ("queueing_metadata.enq_timestamp")
                      ? phv->get_field ("queueing_metadata.enq_timestamp").get_uint ()
                      : 0u;

  enq_qdepth = phv->has_field ("queueing_metadata.enq_qdepth")
                   ? phv->get_field ("queueing_metadata.enq_qdepth").get_uint ()
                   : 0u;

  deq_timedelta = phv->has_field ("queueing_metadata.deq_timedelta")
                      ? phv->get_field ("queueing_metadata.deq_timedelta").get_uint ()
                      : 0u;

  deq_qdepth = phv->has_field ("queueing_metadata.deq_qdepth")
                   ? phv->get_field ("queueing_metadata.deq_qdepth").get_uint ()
                   : 0u;

  ingress_global_timestamp =
      phv->has_field ("intrinsic_metadata.ingress_global_timestamp")
          ? phv->get_field ("intrinsic_metadata.ingress_global_timestamp").get_uint64 ()
          : 0u;

  egress_global_timestamp =
      phv->has_field ("intrinsic_metadata.egress_global_timestamp")
          ? phv->get_field ("intrinsic_metadata.egress_global_timestamp").get_uint64 ()
          : 0u;

  mcast_grp = phv->has_field ("intrinsic_metadata.mcast_grp")
                  ? phv->get_field ("intrinsic_metadata.mcast_grp").get_uint ()
                  : 0u;

  egress_rid = phv->has_field ("intrinsic_metadata.egress_rid")
                   ? phv->get_field ("intrinsic_metadata.egress_rid").get_uint ()
                   : 0u;

  checksum_error = phv->has_field ("standard_metadata.checksum_error")
                       ? phv->get_field ("standard_metadata.checksum_error").get_uint ()
                       : 0u;

  priority = phv->has_field ("standard_metadata.priority")
                 ? phv->get_field ("standard_metadata.priority").get_uint ()
                 : 0u;

  parser_error = phv->has_field ("standard_metadata.parser_error")
                     ? static_cast<StandardMetadata::ParserError> (
                           phv->get_field ("standard_metadata.parser_error").get_uint ())
                     : StandardMetadata::NO_ERROR;
}

void
StandardMetadata::WriteMetadataToBMPacket (std::unique_ptr<bm::Packet> &&bm_packet) const
{
  bm::PHV *phv = bm_packet->get_phv ();
  phv->get_field ("standard_metadata.ingress_port").set (ingress_port);
  phv->get_field ("standard_metadata.egress_spec").set (egress_spec);
  phv->get_field ("standard_metadata.egress_port").set (egress_port);
  phv->get_field ("standard_metadata.instance_type").set (instance_type);
  phv->get_field ("standard_metadata.packet_length").set (packet_length);
  phv->get_field ("queueing_metadata.enq_timestamp").set (enq_timestamp);
  phv->get_field ("queueing_metadata.enq_qdepth").set (enq_qdepth);
  phv->get_field ("queueing_metadata.deq_timedelta").set (deq_timedelta);
  phv->get_field ("queueing_metadata.deq_qdepth").set (deq_qdepth);

  phv->get_field ("intrinsic_metadata.ingress_global_timestamp").set (ingress_global_timestamp);
  phv->get_field ("intrinsic_metadata.egress_global_timestamp").set (egress_global_timestamp);

  phv->get_field ("intrinsic_metadata.mcast_grp").set (mcast_grp);
  phv->get_field ("intrinsic_metadata.egress_rid").set (egress_rid);
  phv->get_field ("standard_metadata.checksum_error").set (checksum_error);
  phv->get_field ("standard_metadata.priority").set (priority);
  phv->get_field ("standard_metadata.parser_error").set (static_cast<uint32_t> (parser_error));
}

P4QueueItem::P4QueueItem (std::unique_ptr<bm::Packet> &&p, PacketType type)
    : m_packet (std::move (p)), m_packetType (type), m_tstamp (Simulator::Now ())
{
  m_metadata = new StandardMetadata ();
}

P4QueueItem::~P4QueueItem ()
{
  NS_LOG_FUNCTION (this);
  delete m_metadata;
}

void
P4QueueItem::SetPacket (std::unique_ptr<bm::Packet> &&p)
{
  m_packet = std::move(p);
}

/**
 * @brief 获取并转移 m_packet 的所有权。
 * @return std::unique_ptr<bm::Packet> 转移的 Packet，m_packet 将变为空。
 */
std::unique_ptr<bm::Packet>
P4QueueItem::GetPacket (void)
{
  return std::move (m_packet);
}

PacketType
P4QueueItem::GetPacketType (void) const
{
  return m_packetType;
}

Time
P4QueueItem::GetTimeStamp (void) const
{
  return m_tstamp;
}

StandardMetadata *
P4QueueItem::GetMetadata (void) const
{
  return m_metadata;
}

void
P4QueueItem::SetMetadata (StandardMetadata *metadata)
{
  m_metadata = metadata;
}

void
P4QueueItem::SetMetadataEgressPort (uint32_t egress_port)
{
  m_port = egress_port;
  m_metadata->egress_port = egress_port;
}

void
P4QueueItem::SetMetadataPriority (uint8_t priority)
{
  // maximum priority is 7 (3 bits)
  m_metadata->priority = (priority >= 8) ? 0 : priority;
  m_priority = m_metadata->priority;
}

void
P4QueueItem::Print (std::ostream &os) const
{
  // os << "P4QueueItem: Packet=" << m_packet;
  // print metadata
  m_metadata->PrintMetadata (os);
}

std::ostream &
operator<< (std::ostream &os, const P4QueueItem &item)
{
  item.Print (os);
  return os;
}

} // namespace ns3
