/*
 * Copyright (c) 2018 Stanford University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Stephen Ibanez <sibanez@stanford.edu>
 *          Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#ifndef P4_NETDEVICE_CORE_H
#define P4_NETDEVICE_CORE_H

#include "p4-net-device.h"
#include "standard-metadata-tag.h"
#include "ns3/prio-queue-disc.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>

namespace ns3
{

// struct PacketInfo
// {
//   int inPort;
//   uint16_t protocol;
//   Address destination;
//   int64_t packet_id;
// };

enum class PacketType
{
    NORMAL,
    RESUBMIT,
    RECIRCULATE,
    SENTINEL // signal for the ingress thread to terminate
};


// class P4NetDevice;

/**
 * \ingroup p4-pipeline
 *
 * Base class for a P4 programmable pipeline.
 */
class P4NetDeviceCore : public bm::Switch
{
  public:
    // by default, swapping is off
    P4NetDeviceCore(P4NetDevice* netDevice);

    ~P4NetDeviceCore();

    // Initialization
    int init(int argc, char* argv[]);
    int InitFromCommandLineOptionsLocal(int argc, char* argv[], bm::TargetParserBasic* tp = nullptr);

    struct PacketInfo
    {
        int in_port;
        uint16_t protocol;
        Address destination;
        int64_t packet_id;
    };

    /**
     * \brief Run the provided CLI commands to populate table entries
     */
    void run_cli(const std::string& commandsFile);

    int receive_(port_t port_num, const char* buffer, int len) override;

    void start_and_return_() override;

    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);

    void parser_ingress_processing(Ptr<Packet> packetIn);

    void enqueue(port_t egress_port, std::unique_ptr<bm::Packet>&& bm_packet, size_t priority);

    void egress_deparser_processing();

    void check_queueing_metadata();

    // int set_egress_priority_queue_depth(size_t port, size_t priority, const size_t depth_pkts);
    // int set_egress_queue_depth(size_t port, const size_t depth_pkts);
    // int set_all_egress_queue_depths(const size_t depth_pkts);
    // int set_egress_priority_queue_rate(size_t port, size_t priority, const uint64_t rate_pps);
    // int set_egress_queue_rate(size_t port, const uint64_t rate_pps);
    // int set_all_egress_queue_rates(const uint64_t rate_pps);

    /**
     * This function P4NetDeviceCore::get_bm_packet converts the NS-3 packet ns_packet to bm::Packet
     * in BMv2 (Behavioral Model v2) format for further processing in the P4 simulation environment
     * in NS-3. Its main functions are to assign a unique packet ID, copy the packet content, and
     * add metadata for the BMv2 packet
     */
    std::unique_ptr<bm::Packet> get_bm_packet(Ptr<Packet> ns3_packet);

    Ptr<Packet> get_ns3_packet(std::unique_ptr<bm::Packet> bm_packet);
    
    Ptr<Packet> get_ns3_packet(std::unique_ptr<bm::Packet> bm_packet, PacketInfo* pkts_info);

    P4NetDeviceCore(const P4NetDeviceCore&) = delete;
    P4NetDeviceCore& operator=(const P4NetDeviceCore&) = delete;
    P4NetDeviceCore(P4NetDeviceCore&&) = delete;
    P4NetDeviceCore&& operator=(P4NetDeviceCore&&) = delete;

  protected:
    static uint64_t packet_id;
    static int thrift_port;

  private:

  enum PktInstanceType {
			PKT_INSTANCE_TYPE_NORMAL,
			PKT_INSTANCE_TYPE_INGRESS_CLONE,
			PKT_INSTANCE_TYPE_EGRESS_CLONE,
			PKT_INSTANCE_TYPE_COALESCED,
			PKT_INSTANCE_TYPE_RECIRC,
			PKT_INSTANCE_TYPE_REPLICATION,
			PKT_INSTANCE_TYPE_RESUBMIT,
		};
  
  std::unordered_map<uint64_t, PacketInfo> uidMap;
  std::unordered_map<uint64_t, uint64_t> reverseUidMap;

  bm::TargetParserBasic * m_argParser; 		    //!< Structure of parsers

  bool skip_tracing = true;          // whether to skip tracing
  bool with_queueing_metadata{true}; // whether to include queueing metadata

  int drop_port = 511;

  size_t nb_queues_per_port{8}; // 3 bit for the queue number, max value is 8

  std::vector<Address> destination_list; //!< list for address, using by index

  PrioQueueDisc queue_buffer;
  P4NetDevice* m_pNetDevice;

};

} // namespace ns3

#endif // !P4_NETDEVICE_CORE_H