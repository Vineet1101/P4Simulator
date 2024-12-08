/***
 * [Deprecated 弃用] 
 * 
 * 这种实现不够灵活，p4switch基于bridge实现，而通常的queue-disc无法自行构建queue-item，而通常不自行实现queue-item。
 * 在一定的探索和思考后：
 * 我认为可以模仿<wifi-mac-queue.h>的实现，分别实现队列和队列项，更加灵活。
 * 而这样我们可以将metadata的定义放在queue-item中，而不是放在item中的packet的tag上，这样更简洁高效。
 * 
 * English:
 * This implementation is not flexible enough. The p4switch is based on the bridge module,
 * and the typical queue-disc cannot construct its own queue-items independently, nor is it common to implement custom queue-items.
 * After some exploration and consideration:
 * I believe we can model the implementation after <wifi-mac-queue.h>, separately implementing the queue and queue-item for greater flexibility.
 * In this way, we can define metadata in the queue-item, rather than in the tag of the packet in the item, which is more concise and efficient.
 * 
 */

#include "ns3/standard-metadata-tag.h"
#include "ns3/log.h"

#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/packet.h>
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StandardMetadataTag");

NS_OBJECT_ENSURE_REGISTERED (StandardMetadataTag);

TypeId
StandardMetadataTag::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::StandardMetadataTag").SetParent<Tag> ().AddConstructor<StandardMetadataTag> ();
  return tid;
}

TypeId
StandardMetadataTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

StandardMetadataTag::StandardMetadataTag () : m_metadata{}
{
}

StandardMetadataTag::StandardMetadataTag (const StandardMetadata &metadata) : m_metadata (metadata)
{
}

uint32_t
StandardMetadataTag::GetSerializedSize (void) const
{
  return sizeof (m_metadata);
}

void
StandardMetadataTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_metadata.ingress_port);
  i.WriteU32 (m_metadata.egress_spec);
  i.WriteU32 (m_metadata.egress_port);
  i.WriteU32 (m_metadata.instance_type);
  i.WriteU32 (m_metadata.packet_length);
  i.WriteU32 (m_metadata.enq_timestamp);
  i.WriteU32 (m_metadata.enq_qdepth);
  i.WriteU32 (m_metadata.deq_timedelta);
  i.WriteU32 (m_metadata.deq_qdepth);

  // The following two fields are 48 bits long, but we only have 64 bits to work with.
  i.WriteU64 (m_metadata.ingress_global_timestamp);
  i.WriteU64 (m_metadata.egress_global_timestamp);

  i.WriteU16 (m_metadata.mcast_grp);
  i.WriteU16 (m_metadata.egress_rid);
  i.WriteU8 (m_metadata.checksum_error);
  i.WriteU8 (m_metadata.priority);
  i.WriteU8 (static_cast<uint8_t> (m_metadata.parser_error));
}

void
StandardMetadataTag::Deserialize (TagBuffer i)
{
  m_metadata.ingress_port = i.ReadU32 ();
  m_metadata.egress_spec = i.ReadU32 ();
  m_metadata.egress_port = i.ReadU32 ();
  m_metadata.instance_type = i.ReadU32 ();
  m_metadata.packet_length = i.ReadU32 ();
  m_metadata.enq_timestamp = i.ReadU32 ();
  m_metadata.enq_qdepth = i.ReadU32 ();
  m_metadata.deq_timedelta = i.ReadU32 ();
  m_metadata.deq_qdepth = i.ReadU32 ();
  m_metadata.ingress_global_timestamp = i.ReadU64 ();
  m_metadata.egress_global_timestamp = i.ReadU64 ();
  m_metadata.mcast_grp = i.ReadU16 ();
  m_metadata.egress_rid = i.ReadU16 ();
  m_metadata.checksum_error = i.ReadU8 ();
  m_metadata.priority = i.ReadU8 ();
  m_metadata.parser_error = static_cast<StandardMetadata::ParserError> (i.ReadU8 ());
}

void
StandardMetadataTag::Print (std::ostream &os) const
{
  os << "StandardMetadataTag - ingress_port: " << m_metadata.ingress_port
     << ", egress_spec: " << m_metadata.egress_spec << ", egress_port: " << m_metadata.egress_port
     << ", instance_type: " << m_metadata.instance_type
     << ", packet_length: " << m_metadata.packet_length;
}

void
StandardMetadataTag::GetMetadataFromBMPacket (std::unique_ptr<bm::Packet> &&bm_packet)
{
  bm::PHV *phv = bm_packet->get_phv ();

  m_metadata.ingress_port = phv->has_field ("standard_metadata.ingress_port")
                                ? phv->get_field ("standard_metadata.ingress_port").get_uint ()
                                : 0u;

  m_metadata.egress_spec = phv->has_field ("standard_metadata.egress_spec")
                               ? phv->get_field ("standard_metadata.egress_spec").get_uint ()
                               : 0u;

  m_metadata.egress_port = phv->has_field ("standard_metadata.egress_port")
                               ? phv->get_field ("standard_metadata.egress_port").get_uint ()
                               : 0u;

  m_metadata.instance_type = phv->has_field ("standard_metadata.instance_type")
                                 ? phv->get_field ("standard_metadata.instance_type").get_uint ()
                                 : 0u;

  m_metadata.packet_length = phv->has_field ("standard_metadata.packet_length")
                                 ? phv->get_field ("standard_metadata.packet_length").get_uint ()
                                 : 0u;

  m_metadata.enq_timestamp = phv->has_field ("queueing_metadata.enq_timestamp")
                                 ? phv->get_field ("queueing_metadata.enq_timestamp").get_uint ()
                                 : 0u;

  m_metadata.enq_qdepth = phv->has_field ("queueing_metadata.enq_qdepth")
                              ? phv->get_field ("queueing_metadata.enq_qdepth").get_uint ()
                              : 0u;

  m_metadata.deq_timedelta = phv->has_field ("queueing_metadata.deq_timedelta")
                                 ? phv->get_field ("queueing_metadata.deq_timedelta").get_uint ()
                                 : 0u;

  m_metadata.deq_qdepth = phv->has_field ("queueing_metadata.deq_qdepth")
                              ? phv->get_field ("queueing_metadata.deq_qdepth").get_uint ()
                              : 0u;

  m_metadata.ingress_global_timestamp =
      phv->has_field ("intrinsic_metadata.ingress_global_timestamp")
          ? phv->get_field ("intrinsic_metadata.ingress_global_timestamp").get_uint64 ()
          : 0u;

  m_metadata.egress_global_timestamp =
      phv->has_field ("intrinsic_metadata.egress_global_timestamp")
          ? phv->get_field ("intrinsic_metadata.egress_global_timestamp").get_uint64 ()
          : 0u;

  m_metadata.mcast_grp = phv->has_field ("intrinsic_metadata.mcast_grp")
                             ? phv->get_field ("intrinsic_metadata.mcast_grp").get_uint ()
                             : 0u;

  m_metadata.egress_rid = phv->has_field ("intrinsic_metadata.egress_rid")
                              ? phv->get_field ("intrinsic_metadata.egress_rid").get_uint ()
                              : 0u;

  m_metadata.checksum_error = phv->has_field ("standard_metadata.checksum_error")
                                  ? phv->get_field ("standard_metadata.checksum_error").get_uint ()
                                  : 0u;

  m_metadata.priority = phv->has_field ("standard_metadata.priority")
                            ? phv->get_field ("standard_metadata.priority").get_uint ()
                            : 0u;

  m_metadata.parser_error = phv->has_field ("standard_metadata.parser_error")
                                ? static_cast<StandardMetadata::ParserError> (
                                      phv->get_field ("standard_metadata.parser_error").get_uint ())
                                : StandardMetadata::NO_ERROR;
}

void
StandardMetadataTag::WriteMetadataToBMPacket (std::unique_ptr<bm::Packet> &&bm_packet) const
{
  bm::PHV *phv = bm_packet->get_phv ();
  phv->get_field ("standard_metadata.ingress_port").set (m_metadata.ingress_port);
  phv->get_field ("standard_metadata.egress_spec").set (m_metadata.egress_spec);
  phv->get_field ("standard_metadata.egress_port").set (m_metadata.egress_port);
  phv->get_field ("standard_metadata.instance_type").set (m_metadata.instance_type);
  phv->get_field ("standard_metadata.packet_length").set (m_metadata.packet_length);
  phv->get_field ("queueing_metadata.enq_timestamp").set (m_metadata.enq_timestamp);
  phv->get_field ("queueing_metadata.enq_qdepth").set (m_metadata.enq_qdepth);
  phv->get_field ("queueing_metadata.deq_timedelta").set (m_metadata.deq_timedelta);
  phv->get_field ("queueing_metadata.deq_qdepth").set (m_metadata.deq_qdepth);

  phv->get_field ("intrinsic_metadata.ingress_global_timestamp")
      .set (m_metadata.ingress_global_timestamp);
  phv->get_field ("intrinsic_metadata.egress_global_timestamp")
      .set (m_metadata.egress_global_timestamp);

  phv->get_field ("intrinsic_metadata.mcast_grp").set (m_metadata.mcast_grp);
  phv->get_field ("intrinsic_metadata.egress_rid").set (m_metadata.egress_rid);
  phv->get_field ("standard_metadata.checksum_error").set (m_metadata.checksum_error);
  phv->get_field ("standard_metadata.priority").set (m_metadata.priority);
  phv->get_field ("standard_metadata.parser_error")
      .set (static_cast<uint32_t> (m_metadata.parser_error));
}

// void StandardMetadataTag::AddTagToPacket(Ptr<Packet> packet) {
//     packet->AddPacketTag(*this);
// }

// void StandardMetadataTag::RemoveTagFromPacket(Ptr<Packet> packet) {
//     packet->RemovePacketTag(*this);
// }

void
StandardMetadataTag::SetMetadata (const StandardMetadata &metadata)
{
  m_metadata = metadata;
}

StandardMetadata
StandardMetadataTag::GetMetadata (void) const
{
  return m_metadata;
}

} // namespace ns3
