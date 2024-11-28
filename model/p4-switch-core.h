/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#ifndef P4_SWITCH_CORE_H
#define P4_SWITCH_CORE_H

#include "bridge-p4-net-device.h"
#include "fifo-queue-disc.h"
#include "standard-metadata-tag.h"

#include "ns3/prio-queue-disc.h"
#include "p4-logic-pri-rl-queue-disc.h"
#include "p4-rr-pri-queue-disc.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
// #include <bm/bm_sim/queueing.h> // rewrite with ns3 queuedisc


namespace ns3
{


struct PacketInfo
    {
        int in_port;
        uint16_t protocol;
        Address destination;
        int64_t packet_id;
    };

/**
 * \ingroup p4-pipeline
 *
 * Base class for a P4 programmable pipeline.
 */
class P4Switch : public bm::Switch
{
  public:
    P4Switch(BridgeP4NetDevice* netDevice);

    ~P4Switch();

    struct MirroringSessionConfig {
			port_t egress_port;
			bool egress_port_valid;
			unsigned int mgid;
			bool mgid_valid;
		};

    enum class PacketType
    {
        NORMAL,
        RESUBMIT,
        RECIRCULATE,
        SENTINEL // signal for the ingress thread to terminate
    };

    /**
     * \brief Run the provided CLI commands to populate table entries
     */
    void run_cli(std::string commandsFile);

    /**
     * \brief Unused
     */
    int receive_(port_t port_num, const char* buffer, int len) override;

    /**
     * \brief Unused
     */
    void start_and_return_() override;

    // void reset_target_state_() override;

    void swap_notify_() override;

    // bool mirroring_add_session(mirror_id_t mirror_id, const MirroringSessionConfig& config);

    // bool mirroring_delete_session(mirror_id_t mirror_id);

    // bool mirroring_get_session(mirror_id_t mirror_id, MirroringSessionConfig* config) const;
    // int set_egress_priority_queue_depth(size_t port, size_t priority, const size_t depth_pkts);
    // int set_egress_queue_depth(size_t port, const size_t depth_pkts);
    // int set_all_egress_queue_depths(const size_t depth_pkts);

    // int set_egress_priority_queue_rate(size_t port, size_t priority, const uint64_t rate_pps);
    // int set_egress_queue_rate(size_t port, const uint64_t rate_pps);
    // int set_all_egress_queue_rates(const uint64_t rate_pps);

    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);
    
    // !!! Deprecated function, see p4-switch-interface.cc for the new init function
    // int init(int argc, char* argv[]);

    /**
     * \brief configure switch with json file
     */
    int InitFromCommandLineOptionsLocal(int argc,
                                        char* argv[]);

    void packets_process_pipeline(Ptr<Packet> packetIn,
                                  int inPort,
                                  uint16_t protocol,
                                  const Address& destination);

    void push_input_buffer(Ptr<Packet> packetIn);

    void push_input_buffer_with_priority(std::unique_ptr<bm::Packet>&& bm_packet, PacketType packet_type);

    void push_transmit_buffer(std::unique_ptr<bm::Packet>&& bm_packet);

    void parser_ingress_processing();

    void enqueue(port_t egress_port, std::unique_ptr<bm::Packet>&& bm_packet);

    void egress_deparser_processing();

    bool mirroring_add_session(int mirror_id, const MirroringSessionConfig& config);

    bool mirroring_delete_session(int mirror_id);

    bool mirroring_get_session(int mirror_id, MirroringSessionConfig* config) const;
    
    void multicast(bm::Packet *packet, unsigned int mgid);

    void check_queueing_metadata();

    int set_egress_priority_queue_depth(size_t port, size_t priority, const size_t depth_pkts);
    int set_egress_queue_depth(size_t port, const size_t depth_pkts);
    int set_all_egress_queue_depths(const size_t depth_pkts);
    int set_egress_priority_queue_rate(size_t port, size_t priority, const uint64_t rate_pps);
    int set_egress_queue_rate(size_t port, const uint64_t rate_pps);
    int set_all_egress_queue_rates(const uint64_t rate_pps);

    /**
     * \brief Set whether to skip tracing, default is true
     */
    void SetSkipTracing(bool skipTracing)
    {
        skip_tracing = skipTracing;
    }

    std::unique_ptr<bm::Packet> get_bm_packet(Ptr<Packet> ns3_packet);
    std::unique_ptr<bm::Packet> get_bm_packet_from_ingress(Ptr<Packet> ns_packet);
    Ptr<Packet> get_ns3_packet(std::unique_ptr<bm::Packet> bm_packet);

    P4Switch(const P4Switch&) = delete;
    P4Switch& operator=(const P4Switch&) = delete;
    P4Switch(P4Switch&&) = delete;
    P4Switch&& operator=(P4Switch&&) = delete;

    static constexpr port_t default_drop_port = 511;

  protected:
    static bm::packet_id_t packet_id;
    static int thrift_port;

  private:
    
    // class MirroringSessions;

    enum PktInstanceType {
			PKT_INSTANCE_TYPE_NORMAL,
			PKT_INSTANCE_TYPE_INGRESS_CLONE,
			PKT_INSTANCE_TYPE_EGRESS_CLONE,
			PKT_INSTANCE_TYPE_COALESCED,
			PKT_INSTANCE_TYPE_RECIRC,
			PKT_INSTANCE_TYPE_REPLICATION,
			PKT_INSTANCE_TYPE_RESUBMIT,
		};

    void copy_field_list_and_set_type(
			const std::unique_ptr<bm::Packet> &packet,
			const std::unique_ptr<bm::Packet> &packet_copy,
			PktInstanceType copy_type, int field_list_id);

    bm::TargetParserBasic * m_argParser; 		    //!< Structure of parsers

    bool skip_tracing = true;          // whether to skip tracing
    bool with_queueing_metadata{true}; // whether to include queueing metadata

    size_t nb_queues_per_port{8}; // 3 bit for the queue number, max value is 8
    
    // std::unique_ptr<MirroringSessions> mirroring_sessions;

    std::vector<Address> destination_list; //!< list for address, using by index

    Ptr<PrioQueueDisc> input_buffer;
    Ptr<NSP4PriQueueDisc> queue_buffer;
    Ptr<FifoQueueDisc> transmit_buffer; 

    BridgeP4NetDevice* m_pNetDevice;
};

} // namespace ns3

#endif // !P4_SWITCH_CORE_H