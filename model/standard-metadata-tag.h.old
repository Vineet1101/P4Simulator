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

#ifndef STANDARD_METADATA_TAG_H
#define STANDARD_METADATA_TAG_H

#include "ns3/packet.h"
#include "ns3/tag.h"

#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/packet.h>

#include <cstdint>
#include <iomanip> // 用于 std::setw

namespace ns3 {

#ifndef V1MODEL_VERSION
#define V1MODEL_VERSION 20200408
#endif

/**
 * \ingroup p4sim
 * \brief Standard metadata structure. This structure is used to store metadata that is added to
 * packets by the p4 switch. The metadata is added to the packet as a tag. Check the V1Model in
 * https://github.com/p4lang/p4c/blob/main/p4include/v1model.p4 for more information.
 */
struct StandardMetadata
{
#if V1MODEL_VERSION >= 20200408
  uint32_t ingress_port;
  uint32_t egress_spec;
  uint32_t egress_port;
#else
  uint16_t ingress_port : 9;
  uint16_t egress_spec : 9;
  uint16_t egress_port : 9;
#endif

  uint32_t instance_type;
  uint32_t packet_length;

  uint32_t enq_timestamp;
  uint32_t enq_qdepth : 19;
  uint32_t deq_timedelta;
  uint32_t deq_qdepth : 19;

  uint64_t ingress_global_timestamp : 48;
  uint64_t egress_global_timestamp : 48;
  uint16_t mcast_grp;
  uint16_t egress_rid;
  uint8_t checksum_error : 1;
  uint8_t priority : 3;

  enum ParserError { NO_ERROR, ERROR_CHECKSUM, ERROR_OTHER } parser_error;

  void PrintMetadata (std::ostream &os) const;
};

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

/**
 * \ingroup p4sim
 * \brief Tag to store standard metadata. The p4 switch will add metadata to each packet entering
 * the switch. The metadata originally existing in ns3 exists to save the header and structure,
 * which is not the same as the metadata in p4. In order to facilitate the transmission of metadata
 * with the packet, the tag function is used here.
 */
class StandardMetadataTag : public Tag
{
public:
  StandardMetadataTag ();
  StandardMetadataTag (const StandardMetadata &metadata);

  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const override;
  uint32_t GetSerializedSize (void) const override;
  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;
  void Print (std::ostream &os) const override;

  /**
     * \brief Get metadata from a bm::Packet and store it in the tag
     */
  void GetMetadataFromBMPacket (std::unique_ptr<bm::Packet> &&bm_packet);

  /**
     * \brief Write metadata to a bm::Packet from the tag
     */
  void WriteMetadataToBMPacket (std::unique_ptr<bm::Packet> &&bm_packet) const;

  // /**
  //  * \brief Add the tag with metadata for a packet
  //  */
  // void AddTagToPacket(Ptr<Packet> packet) const;

  // /**
  //  * \brief Remove the tag with metadata from a packet
  //  */
  // void RemoveTagFromPacket(Ptr<Packet> packet) const;

  void SetMetadata (const StandardMetadata &metadata);
  StandardMetadata GetMetadata (void) const;

private:
  StandardMetadata m_metadata;
};

} // namespace ns3

#endif // STANDARD_METADATA_TAG_H
