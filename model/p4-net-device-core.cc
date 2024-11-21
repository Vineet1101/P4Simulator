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
 *
 * \TODO GetNanoSeconds or GetMicroSeconds?
 *
 */

#include "p4-net-device-core.h"

// #include "queue-item.h"
#include "standard-metadata-tag.h"
#include "ns3/ethernet-header.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"

#include "register_access.h"
#include "global.h"

#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv.h>
#include <bm/bm_sim/tables.h>
#include <unordered_map>

namespace ns3
{

namespace
{

struct hash_ex
{
    uint32_t operator()(const char* buf, size_t s) const
    {
        const uint32_t p = 16777619;
        uint32_t hash = 2166136261;

        for (size_t i = 0; i < s; i++)
            hash = (hash ^ buf[i]) * p;

        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        return static_cast<uint32_t>(hash);
    }
};

struct bmv2_hash
{
    uint64_t operator()(const char* buf, size_t s) const
    {
        return bm::hash::xxh64(buf, s);
    }
};

} // namespace

// if REGISTER_HASH calls placed in the anonymous namespace, some compiler can
// give an unused variable warning
REGISTER_HASH(hash_ex);
REGISTER_HASH(bmv2_hash);

// initialize static attributes

// Single mapping table, bm::Packet UID -> latest ns3::Packet UID
// For each new ns3::Packet created, it will have a new UID for the ns3::Packet, but we need to
// keep the same UID for the bm::Packet, so we need to keep a mapping between the ns3::Packet
// UID and bm::Packet UID, and after new ns3 packets creates, update the new value of
// ns3::Packet UID for mapping.
uint64_t P4NetDeviceCore::packet_id = 0;
// int P4NetDeviceCore::thrift_port = 8090; // @TODO - Remove this

P4NetDeviceCore::P4NetDeviceCore(P4NetDevice* netDevice){
    NS_LOG_FUNCTION(this);

    m_pNetDevice = netDevice;

    // Set custom Priomap: {0, 1, 2, 3, 4, 5, 6, ..., 15}, but we only use 0-7 bands
    Priomap priomap;
    for (uint8_t i = 0; i < 16; ++i) {
        if (i < 8) {
            priomap[i] = 7 - i; // Map priority i to its own band for priorities below 8
        } else {
            priomap[i] = 7; // Map priority 8 and above to band 0
        }
    }
    queue_buffer.SetAttribute("Priomap", PriomapValue(priomap));

}

int
P4NetDeviceCore::init(int argc, char* argv[])
{
    NS_LOG_FUNCTION(this);
    int status = 0;

    //  Several methods of populating flowtable
    if (P4GlobalVar::g_populateFlowTableWay == LOCAL_CALL) {
        // Handle "exact" match table initialization only
        status = InitFromCommandLineOptionsLocal(argc, argv, m_argParser);
    } else if (P4GlobalVar::g_populateFlowTableWay == RUNTIME_CLI) {
        /**
         * @brief start thrift server , use runtime_CLI populate flowtable
         * This method is from src
         * This will connect to the simple_switch thrift server and input the command.
         * by now the bm::switch and the bm::simple_switch is not the same thing, so
         *  the "sswitch_runtime::get_handler()" by now can not use. @todo -mingyu
         */
        
        // status = this->init_from_command_line_options(argc, argv, m_argParser);
        // int thriftPort = this->get_runtime_port();
        // std::cout << "thrift port : " << thriftPort << std::endl;
        // bm_runtime::start_server(this, thriftPort);
        // //@todo BUG: THIS MAY CHANGED THE API
        // using ::sswitch_runtime::SimpleSwitchIf;
        // using ::sswitch_runtime::SimpleSwitchProcessor;
        // bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>(
        //         "simple_switch", sswitch_runtime::get_handler(this));
        
    } else if (P4GlobalVar::g_populateFlowTableWay == NS3PIFOTM) {
        // JSON file-based flow table population, NS3-PIFO-TM based
        static int thrift_port = 8090;
        bm::OptionsParser opt_parser;
        opt_parser.config_file_path = P4GlobalVar::g_p4JsonPath;
        opt_parser.debugger_addr = "ipc:///tmp/bmv2-" + std::to_string(thrift_port) + "-debug.ipc";
        opt_parser.notifications_addr =
            "ipc:///tmp/bmv2-" + std::to_string(thrift_port) + "-notifications.ipc";
        opt_parser.file_logger = "/tmp/bmv2-" + std::to_string(thrift_port) + "-pipeline.log";
        opt_parser.thrift_port = thrift_port++;
        opt_parser.console_logging = true;

        status = init_from_options_parser(opt_parser);
        if (status == 0)
        {
            int port = get_runtime_port();
            bm_runtime::start_server(this, port);
            std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string(port) + " < " +
                              P4GlobalVar::g_flowTablePath + " > /dev/null 2>&1";
            if (std::system(cmd.c_str()) != 0)
            {
                std::cerr << "Error executing command." << std::endl;
            }
        }
    } else {
        NS_LOG_ERROR("ERROR: P4 Model switch init failed in P4NetDeviceCore::init.");
        return -1;
    }


    if (status != 0)
    {
        NS_LOG_LOGIC("ERROR: P4 Model switch init failed in P4NetDeviceCore::init.");
        std::exit(status);
    }
    return 0;
}

int
P4NetDeviceCore::InitFromCommandLineOptionsLocal(int argc, char* argv[], bm::TargetParserBasic* tp)
{
    bm::OptionsParser parser;
    parser.parse(argc, argv, tp);

    // create a dummy transport
    std::shared_ptr<bm::TransportIface> transport =
        std::shared_ptr<bm::TransportIface>(bm::TransportIface::make_dummy());

    int status = 0;
    if (parser.no_p4)
        // with out p4-json, acctually the switch will wait for the configuration(p4-json) before
        // work
        status = init_objects_empty(parser.device_id, transport);
    else
        // load p4 configuration files xxxx.json to switch
        status = init_objects(parser.config_file_path, parser.device_id, transport);
    return status;
}

void
P4NetDeviceCore::run_cli(const std::string& commandsFile)
{
    int port = get_runtime_port();
    bm_runtime::start_server(this, port);
    start_and_return();

    std::this_thread::sleep_for(std::chrono::seconds(5)); // @TODO - Remove this

    // Run the CLI commands to populate table entries
    std::string cmd = "run_bmv2_CLI --thrift_port " + std::to_string(port) + " " + commandsFile;
    std::system(cmd.c_str());
}

int
P4NetDeviceCore::receive_(port_t port_num, const char* buffer, int len)
{
    NS_LOG_FUNCTION(this);
    return 0;
}

void
P4NetDeviceCore::start_and_return_()
{
    NS_LOG_FUNCTION("p4_switch has been start");
    check_queueing_metadata();
}

void P4NetDeviceCore::check_queueing_metadata()
{
    // TODO(antonin): add qid in required fields
    bool enq_timestamp_e = field_exists("queueing_metadata", "enq_timestamp");
    bool enq_qdepth_e = field_exists("queueing_metadata", "enq_qdepth");
    bool deq_timedelta_e = field_exists("queueing_metadata", "deq_timedelta");
    bool deq_qdepth_e = field_exists("queueing_metadata", "deq_qdepth");
    if (enq_timestamp_e || enq_qdepth_e || deq_timedelta_e || deq_qdepth_e)
    {
        if (enq_timestamp_e && enq_qdepth_e && deq_timedelta_e && deq_qdepth_e)
        {
            with_queueing_metadata = true;
            return;
        }
        else
        {
            NS_LOG_WARN("Your JSON input defines some but not all queueing metadata fields");
        }
    }
    else
    {
        NS_LOG_WARN("Your JSON input does not define any queueing metadata fields");
    }
    with_queueing_metadata = false;
}

int
P4NetDeviceCore::ReceivePacket(Ptr<Packet> packetIn,
                               int inPort,
                               uint16_t protocol,
                               const Address& destination)
{
    NS_LOG_FUNCTION(this);

    // save the ns3 packet uid and the port, protocol, destination
    uint64_t ns3Uid = packetIn->GetUid();
    PacketInfo pkts_info = {inPort, protocol, destination, 0};
    uidMap[ns3Uid] = pkts_info;

    // process the packet in the pipeline
    this->parser_ingress_processing(packetIn);
    return 0;
}

void
P4NetDeviceCore::enqueue(port_t egress_port,
                         std::unique_ptr<bm::Packet>&& bm_packet,
                         size_t priority)
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> ns_packet = get_ns3_packet(std::move(bm_packet));

    // add priority tag for the packet
    ns3::SocketPriorityTag priorityTag;
    priorityTag.SetPriority(priority); // Set the priority value
    ns_packet->AddPacketTag(priorityTag); // Attach the tag to the packet

    // Enqueue the packet in the queue buffer
    Ptr<QueueItem> queue_item = CreateObject<QueueItem>(ns_packet);

    if (queue_buffer.Enqueue(queue_item)) 
    {
        // enqueue success
        NS_LOG_INFO("Packet enqueued in P4QueueDisc, Port: " << egress_port
                                                             << ", Priority: " << priority);
    }
    else
    {
        NS_LOG_WARN("QueueDisc P4QueueDisc is full, dropping packet, Port: "
                    << egress_port << ", Priority: " << priority);
    }
}

void
P4NetDeviceCore::parser_ingress_processing(Ptr<Packet> packetIn)
{
    NS_LOG_FUNCTION(this);

    auto bm_packet = get_bm_packet(packetIn);

    bm::Parser* parser = this->get_parser("parser");
    bm::Pipeline* ingress_mau = this->get_pipeline("ingress");
    bm::PHV* phv = bm_packet->get_phv();

    parser->parse(bm_packet.get());

    if (phv->has_field("standard_metadata.parser_error"))
    {
        phv->get_field("standard_metadata.parser_error").set(bm_packet->get_error_code().get());
    }
    if (phv->has_field("standard_metadata.checksum_error"))
    {
        phv->get_field("standard_metadata.checksum_error")
            .set(bm_packet->get_checksum_error() ? 1 : 0);
    }

    ingress_mau->apply(bm_packet.get());

    bm_packet->reset_exit();

    bm::Field& f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    port_t egress_spec = f_egress_spec.get_uint();
    port_t egress_port = egress_spec;
    NS_LOG_DEBUG("Egress port is " << egress_port);

    if (egress_port == drop_port)
    {
        NS_LOG_DEBUG("Dropping packet at the end of ingress");
        return;
    }

    auto& f_instance_type = phv->get_field("standard_metadata.instance_type");
    f_instance_type.set(PKT_INSTANCE_TYPE_NORMAL);

    bm_packet->set_egress_port(egress_port);

    size_t priority = 0; // default priority
    if (with_queueing_metadata)
    {
        phv->get_field("queueing_metadata.enq_timestamp").set(Simulator::Now().GetMicroSeconds());

        priority = phv->has_field("intrinsic_metadata.priority")
                       ? phv->get_field("intrinsic_metadata.priority").get<size_t>()
                       : 0u;
        if (priority >= nb_queues_per_port)
        {
            NS_LOG_ERROR("Priority out of range, dropping packet");
            return;
        }
        phv->get_field("queueing_metadata.enq_qdepth")
            .set(queue_buffer.GetQueueDiscClass(priority)->GetQueueDisc()->GetNPackets()); // @TODO
    }

    enqueue(egress_port, std::move(bm_packet), priority);
}

void
P4NetDeviceCore::egress_deparser_processing()
{
    NS_LOG_FUNCTION("Dequeue packet from QueueBuffer");
    // Here need the ID.
    Ptr<QueueDiscItem> item =
        queue_buffer.Dequeue(); // \TODO get the NetDevice ID (egress port) and priority of the pacekts
    if (item == nullptr)
    {
        NS_LOG_WARN("GetQueueBuffer is empty, no packet to dequeue");
        return;
    }
    Ptr<Packet> dequeued_ns_packet = item->GetPacket();
    NS_LOG_INFO("Packet dequeued from GetQueueBuffer");
    auto bm_packet = get_bm_packet(dequeued_ns_packet);

    NS_LOG_FUNCTION("Egress processing for the packet");
    bm::PHV* phv = bm_packet->get_phv();
    bm::Pipeline* egress_mau = this->get_pipeline("egress");
    bm::Deparser* deparser = this->get_deparser("deparser");

    if (phv->has_field("intrinsic_metadata.egress_global_timestamp"))
    {
        phv->get_field("intrinsic_metadata.egress_global_timestamp")
            .set(Simulator::Now().GetMicroSeconds());
    }

    if (with_queueing_metadata)
    {
        uint64_t enq_timestamp = phv->get_field("queueing_metadata.enq_timestamp").get<uint64_t>();
        uint64_t now = Simulator::Now().GetMicroSeconds();
        phv->get_field("queueing_metadata.deq_timedelta").set(now - enq_timestamp);

        size_t priority = phv->has_field("intrinsic_metadata.priority")
                       ? phv->get_field("intrinsic_metadata.priority").get<size_t>()
                       : 0u;
        if (priority >= nb_queues_per_port)
        {
            NS_LOG_ERROR("Priority out of range (nb_queues_per_port = " << nb_queues_per_port
                                                                        << "), dropping packet");
            return;
        }

        // \TODO Get the port of the packet

        phv->get_field("queueing_metadata.deq_qdepth")
            .set(this->queue_buffer[port][priority].GetNPackets());
        if (phv->has_field("queueing_metadata.qid"))
        {
            auto& qid_f = phv->get_field("queueing_metadata.qid");
            qid_f.set(nb_queues_per_port - 1 - priority);
        }
    }

    int port = 0; // for each NetDevice, the port is only one
    phv->get_field("standard_metadata.egress_port").set(port);

    bm::Field& f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    f_egress_spec.set(0);

    phv->get_field("standard_metadata.packet_length")
        .set(bm_packet->get_register(RegisterAccess::PACKET_LENGTH_REG_IDX));

    egress_mau->apply(bm_packet.get());

    // TODO(antonin): should not be done like this in egress pipeline
    port_t egress_spec = f_egress_spec.get_uint();
    if (egress_spec == drop_port)
    {
        // drop packet
        NS_LOG_DEBUG("Dropping packet at the end of egress");
        return;
    }

    deparser->deparse(bm_packet.get());

    PacketInfo pkts_info;
    Ptr<Packet> ns_packet = get_ns3_packet(std::move(bm_packet), &pkts_info);

    // send pkts out from the NetDevice
    EthernetHeader eeh;
    ns_packet->RemoveHeader(eeh);
    m_pNetDevice->Send(ns_packet, pkts_info.destination, pkts_info.protocol);
}

// std::unique_ptr<bm::Packet>
// P4NetDeviceCore::get_bm_packet(Ptr<Packet> ns_packet)
// {
//     NS_LOG_FUNCTION(this);

//     // begin to set for the UID mapping
//     uint64_t bmUid = ++packet_id;
//     uint64_t ns3Uid = ns_packet->GetUid();
//     PacketInfo pkts_info = uidMap[ns3Uid];
//     pkts_info.bmUid = bmUid;
//     uidMap[ns3Uid] = pkts_info;
//     reverseUidMap[bmUid] = ns3Uid;

//     int in_port = pkts_info.in_port;

//     int len = ns_packet->GetSize();
//     uint8_t* pkt_buffer = new uint8_t[ns3_pkt_length];
//     ns_packet->CopyData(pkt_buffer, ns3_pkt_length);

//     // we limit the packet buffer to original size + 512 bytes, which means we
//     // cannot add more than 512 bytes of header data to the packet, which should
//     // be more than enough
//     std::unique_ptr<bm::Packet> bm_packet =
//         new_packet_ptr(in_port,
//                        bmUid,
//                        bm::PacketBuffer(ns3_pkt_length + 512, (char*)pkt_buffer, ns3_pkt_length));
//     delete[] pkt_buffer;

//     // Add MetaData for all the packets comes in the P4 switch
//     StandardMetadataTag metadata_tag;
//     metadata_tag.WriteMetadataToBMPacket(bm_packet);

//     return bm_packet;
// }

std::unique_ptr<bm::Packet> P4NetDeviceCore::get_bm_packet(Ptr<Packet> ns_packet)
{
    NS_LOG_FUNCTION(this);

    // Set bmUid and ns3Uid for UID mapping
    uint64_t bmUid = ++packet_id;
    uint64_t ns3Uid = ns_packet->GetUid();
    PacketInfo pkts_info = uidMap[ns3Uid];
    pkts_info.packet_id = bmUid;
    uidMap[ns3Uid] = pkts_info;
    reverseUidMap[bmUid] = ns3Uid;

    int in_port = pkts_info.in_port;
    int len = ns_packet->GetSize();
    
    // Use std::vector instead of new[] to automatically manage memory
    uint8_t* pkt_buffer = new uint8_t[len];
    ns_packet->CopyData(pkt_buffer, len);

    bm::PacketBuffer buffer(len + 512, (char*)pkt_buffer, len);

    std::unique_ptr<bm::Packet> bm_packet =
        new_packet_ptr(in_port, bmUid, len, std::move(buffer));

    delete[] pkt_buffer;
    
    // Add metadata
    StandardMetadataTag metadata_tag;
    metadata_tag.WriteMetadataToBMPacket(std::move(bm_packet));

    return bm_packet;
}

Ptr<Packet>
P4NetDeviceCore::get_ns3_packet(std::unique_ptr<bm::Packet> bm_packet)
{
    // Create a new ns3::Packet using the data buffer
    char* bm_buf = bm_packet.get()->data();
    size_t len = bm_packet.get()->get_data_size();
    Ptr<Packet> ns_packet = Create<Packet>((uint8_t*)(bm_buf), len);

    // Update the mapping table to map bm::Packet UID to the new ns3::Packet UID
    uint64_t bmUid = bm_packet.get()->get_packet_id();
    uint64_t ns3Uid = ns_packet->GetUid();

    auto it = reverseUidMap.find(bmUid);
    if (it != reverseUidMap.end())
    {
        uint64_t oldNs3Uid = it->second;
        PacketInfo pkts_info = uidMap[oldNs3Uid];

        if (pkts_info.packet_id != bmUid)
        {
            NS_LOG_ERROR("The bm::Packet UID in the mapping table is not consistent.");
        }

        uidMap[ns3Uid] = pkts_info;
        uidMap.erase(oldNs3Uid);       // remove the old mapping
        reverseUidMap[bmUid] = ns3Uid; // update
    }
    else
    {
        NS_LOG_ERROR("Can not find the bm::Packet UID in the mapping table.");
    }

    // Add Tag with Metadata information to the ns_packet
    StandardMetadataTag metadata_tag;
    metadata_tag.GetMetadataFromBMPacket(std::move(bm_packet));
    ns_packet->AddPacketTag(metadata_tag);

    return ns_packet;
}

Ptr<Packet>
P4NetDeviceCore::get_ns3_packet(std::unique_ptr<bm::Packet> bm_packet, PacketInfo* pkts_info)
{
    // Create a new ns3::Packet using the data buffer
    char* bm_buf = bm_packet.get()->data();
    size_t len = bm_packet.get()->get_data_size();
    Ptr<Packet> ns_packet = Create<Packet>((uint8_t*)(bm_buf), len);

    // Get pkts_info from the mapping table
    uint64_t bmUid = bm_packet.get()->get_packet_id();
    auto it = reverseUidMap.find(bmUid);
    if (it != reverseUidMap.end())
    {
        uint64_t oldNs3Uid = it->second;

        // Directly update the value at the pointer pkts_info
        *pkts_info = uidMap[oldNs3Uid];

        if (pkts_info->packet_id != bmUid)
        {
            NS_LOG_ERROR("The bm::Packet UID in the mapping table is not consistent.");
        }

        // Clean the old mapping
        uidMap.erase(oldNs3Uid);
        reverseUidMap.erase(bmUid);
    }
    else
    {
        NS_LOG_ERROR("Cannot find the bm::Packet UID in the mapping table.");
    }

    // Return the ns-3 packet
    return ns_packet;
}

// int
// P4NetDeviceCore::set_egress_priority_queue_depth(size_t port,
//                                                  size_t priority,
//                                                  const size_t depth_pkts)
// {
//     egress_buffers.set_capacity(port, priority, depth_pkts);
//     return 0;
// }

// int
// P4NetDeviceCore::set_egress_queue_depth(size_t port, const size_t depth_pkts)
// {
//     egress_buffers.set_capacity(port, depth_pkts);
//     return 0;
// }

// int
// P4NetDeviceCore::set_all_egress_queue_depths(const size_t depth_pkts)
// {
//     egress_buffers.set_capacity_for_all(depth_pkts);
//     return 0;
// }

// int
// P4NetDeviceCore::set_egress_priority_queue_rate(size_t port,
//                                                 size_t priority,
//                                                 const uint64_t rate_pps)
// {
//     egress_buffers.set_rate(port, priority, rate_pps);
//     return 0;
// }

// int
// P4NetDeviceCore::set_egress_queue_rate(size_t port, const uint64_t rate_pps)
// {
//     egress_buffers.set_rate(port, rate_pps);
//     return 0;
// }

// int
// P4NetDeviceCore::set_all_egress_queue_rates(const uint64_t rate_pps)
// {
//     egress_buffers.set_rate_for_all(rate_pps);
//     return 0;
// }

} // namespace ns3
