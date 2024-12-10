/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Custom queue item for P4-based simulation in NS-3
 * 这个item是用来存储ns3::packet，timestamp和metadata
 * 当bm::Packet被转换为ns3::Packet时，metadata无法添加到ns3::Packet中，而ns3::packet在switch中仅出现于队列中。
 * 因此，我们需要一个item来存储metadata
 */

/* Note 1: More details about the definition of v1model architecture
 * can be found at the location below.
 *
 * https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md
 *
 * Note 3: There are at least some P4_14 implementations where
 * invoking a generate_digest operation on a field_list will create a
 * message to the control plane that contains the values of those
 * fields when the ingress control is finished executing, which can be
 * different than the values those fields have at the time the
 * generate_digest operation is invoked in the program, if those field
 * values are changed later in the execution of the P4_14 ingress
 * control.
 *
 * The P4_16 plus v1model implementation should always create digest
 * messages that contain the values of the specified fields at the
 * time that the digest extern function is called.  Thus if a P4_14
 * program expecting the behavior described above is compiled using
 * p4c, it may behave differently.
 */

#ifndef P4_QUEUE_ITEM_H
#define P4_QUEUE_ITEM_H

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"

#include <bm/bm_sim/packet.h>

namespace ns3 {

#ifndef V1MODEL_VERSION
#define V1MODEL_VERSION 20200408
#endif

enum class PacketType {
  NORMAL,
  RESUBMIT,
  RECIRCULATE,
  SENTINEL // signal for the ingress thread to terminate
};

std::ostream &operator<< (std::ostream &os, PacketType type);

/**
 * \ingroup p4sim
 * \brief Standard metadata structure. This structure is used to store metadata that is added to
 * packets by the p4 switch. The metadata is added to the packet as a tag. Check the V1Model in
 * https://github.com/p4lang/p4c/blob/main/p4include/v1model.p4 for more information.
 */
struct StandardMetadata
{
  // \@TODO add ebpf_model, pnat_model, ubpf_model, xdp_model and etc.

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

  //
  // @alias is used to generate the field_alias section of the BMV2 JSON.
  // Field alias creates a mapping from the metadata name in P4 program to
  // the behavioral model's internal metadata name. Here we use it to
  // expose all metadata supported by simple switch to the user through
  // standard_metadata_t.
  //
  // flattening fields that exist in bmv2-ss

  // queueing metadata
  uint32_t enq_timestamp; // @alias("queueing_metadata.enq_timestamp")
  uint32_t enq_qdepth : 19; // @alias("queueing_metadata.enq_qdepth")
  uint32_t deq_timedelta; // @alias("queueing_metadata.deq_timedelta")
  uint32_t deq_qdepth : 19; // @alias("queueing_metadata.deq_qdepth")

  // intrinsic metadata
  uint64_t ingress_global_timestamp : 48; //@alias("intrinsic_metadata.ingress_global_timestamp")
  uint64_t egress_global_timestamp : 48; // @alias("intrinsic_metadata.egress_global_timestamp")
  /// multicast group id (key for the mcast replication table)
  uint16_t mcast_grp; // @alias("intrinsic_metadata.mcast_grp")
  /// Replication ID for multicast
  uint16_t egress_rid; // @alias("intrinsic_metadata.egress_rid")
  /// Indicates that a verify_checksum() method has failed.
  /// 1 if a checksum error was found, otherwise 0.
  uint8_t checksum_error : 1;
  /// Error produced by parsing
  enum ParserError { NO_ERROR, ERROR_CHECKSUM, ERROR_OTHER } parser_error;
  /// set packet priority
  uint8_t priority : 3;

  // ns3 simulation add-ons
  PacketType packet_type; // Assume default constructor exists
  uint16_t ns3_inport;
  uint64_t ns3_uid;
  uint64_t bm_uid; // Initialize to 0
  uint16_t ns3_protocol;
  Address ns3_destination; // Assume default constructor exists

  StandardMetadata ()
#if V1MODEL_VERSION >= 20200408
      : ingress_port (0),
        egress_spec (0),
        egress_port (0),
#else
      : ingress_port (0),
        egress_spec (0),
        egress_port (0),
#endif
        instance_type (0),
        packet_length (0),
        enq_timestamp (0),
        enq_qdepth (0),
        deq_timedelta (0),
        deq_qdepth (0),
        ingress_global_timestamp (0),
        egress_global_timestamp (0),
        mcast_grp (0),
        egress_rid (0),
        checksum_error (0),
        parser_error (NO_ERROR),
        priority (0),
        packet_type (PacketType::NORMAL), // Assume default constructor exists
        ns3_inport (0),
        ns3_uid (0),
        bm_uid (0),
        ns3_protocol (0),
        ns3_destination (Address ()) // Assume default constructor exists
  {
  }

  void PrintMetadata (std::ostream &os) const;
  void GetMetadataFromBMPacket (std::unique_ptr<bm::Packet> &&bm_packet);
  void WriteMetadataToBMPacket (std::unique_ptr<bm::Packet> &&bm_packet) const;
};

/**
 * \ingroup p4sim
 *
 * P4QueueItem stores packets along with their metadate and timestamps.
 */
class P4QueueItem : public SimpleRefCount<P4QueueItem>
{
public:
  /**
   * \brief Constructor
   * \param p The packet included in the queue item
   */
  P4QueueItem (Ptr<Packet> p, PacketType type);

  /**
   * \brief Destructor
   */
  virtual ~P4QueueItem ();

  /**
   * \brief Get the packet stored in this item
   * \return The packet stored in this item
   */
  Ptr<Packet> GetPacket (void) const;

  /**
   * \brief Get the timestamp included in this item
   * \return The timestamp associated with this item
   */
  Time GetTimeStamp (void) const;

  /**
   * \brief Get the priority of the packet
   * \return The priority of the packet
   */
  PacketType GetPacketType (void) const;
  /**
   * \brief Print the item contents
   * \param os Output stream to print to
   */
  virtual void Print (std::ostream &os) const;

  /**
     * \brief Get the metadata associated with the packet
     * \return The metadata associated with the packet
     */
  StandardMetadata *GetMetadata (void) const;

  void SetMetadataEgressPort (uint32_t egress_port);
  void SetMetadataPriority (uint8_t priority);

  /**
     * \brief Set the metadata associated with the packet
     * \param metadata The metadata to associate with the packet
     */
  void SetMetadata (StandardMetadata *metadata);

private:
  Ptr<Packet> m_packet; //!< The packet contained in this queue item
  PacketType m_packetType; //!< The type of the packet
  int m_priority{0}; //!< The priority of the packet
  int m_port{0}; //!< The port of the egress switch
  Time m_tstamp; //!< Timestamp when the packet was enqueued
  StandardMetadata *m_metadata; //!< Metadata associated with the packet
};

std::ostream &operator<< (std::ostream &os, const P4QueueItem &item);

} // namespace ns3

#endif /* P4_QUEUE_ITEM_H */
