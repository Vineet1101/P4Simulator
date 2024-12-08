/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Custom queue item for P4-based simulation in NS-3
 * 这个item是用来存储ns3::packet，timestamp和metadata
 * 当bm::Packet被转换为ns3::Packet时，metadata无法添加到ns3::Packet中，而ns3::packet在switch中仅出现于队列中。
 * 因此，我们需要一个item来存储metadata
 */

#ifndef P4_QUEUE_ITEM_H
#define P4_QUEUE_ITEM_H

#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"
#include "ns3/standard-metadata-tag.h"

#include <bm/bm_sim/packet.h>

namespace ns3 {

enum class PacketType {
  NORMAL,
  RESUBMIT,
  RECIRCULATE,
  SENTINEL // signal for the ingress thread to terminate
};

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

  // other information for simulation
  PacketType packet_type; // packet type

  enum ParserError { NO_ERROR, ERROR_CHECKSUM, ERROR_OTHER } parser_error;

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
   * \param tstamp The timestamp associated with the packet
   */
  P4QueueItem (Ptr<const Packet> p, Time tstamp);

  /**
   * \brief Destructor
   */
  virtual ~P4QueueItem ();

  /**
   * \brief Get the packet stored in this item
   * \return The packet stored in this item
   */
  Ptr<const Packet> GetPacket (void) const;

  /**
   * \brief Get the timestamp included in this item
   * \return The timestamp associated with this item
   */
  Time GetTimeStamp (void) const;

  /**
   * \brief Print the item contents
   * \param os Output stream to print to
   */
  virtual void Print (std::ostream &os) const;

  /**
     * \brief Get the metadata associated with the packet
     * \return The metadata associated with the packet
     */
  StandardMetadata GetMetadata (void) const;

  /**
     * \brief Set the metadata associated with the packet
     * \param metadata The metadata to associate with the packet
     */
  void SetMetadata (const StandardMetadata &metadata);

private:
  Ptr<const Packet> m_packet; //!< The packet contained in this queue item
  int m_priority; //!< The priority of the packet
  int m_port; //!< The port of the egress switch
  Time m_tstamp; //!< Timestamp when the packet was enqueued
  StandardMetadata m_metadata; //!< Metadata associated with the packet
};

/**
 * \brief Stream insertion operator for P4QueueItem
 * \param os Output stream
 * \param item The queue item to print
 * \return Reference to the output stream
 */
std::ostream &operator<< (std::ostream &os, const P4QueueItem &item);

} // namespace ns3

#endif /* P4_QUEUE_ITEM_H */
