/*
 * Copyright (c)
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
 * Author: Jiasong Bai
 * Modified: Mingyu Ma<mingyu.ma@tu-dresden.de>
 */

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF

#ifdef Mutex
#undef Mutex
#endif

#ifdef registry_t
#undef registry_t
#endif

#include <bm/spdlog/spdlog.h>
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_DEBUG

#include "ns3/ethernet-header.h"
#include "ns3/log.h"
#include "ns3/p4-core-pipeline.h"
#include "ns3/register_access.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"

#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/phv.h>

NS_LOG_COMPONENT_DEFINE("P4CorePipeline");

namespace ns3
{

namespace
{

struct hash_ex_v1model_pipeline
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

struct bmv2_hash_v1model_pipeline
{
    uint64_t operator()(const char* buf, size_t s) const
    {
        return bm::hash::xxh64(buf, s);
    }
};

} // namespace

// if REGISTER_HASH calls placed in the anonymous namespace, some compiler can
// give an unused variable warning
REGISTER_HASH(hash_ex_v1model_pipeline);
REGISTER_HASH(bmv2_hash_v1model_pipeline);

P4CorePipeline::P4CorePipeline(P4SwitchNetDevice* netDevice,
                               bool enableSwap,
                               bool enableTracing,
                               uint32_t drop_port)
    : bm::Switch(enableSwap),
      m_enableTracing(enableTracing),
      m_enableSwap(enableSwap),
      m_dropPort(drop_port)
{
    m_packetId = 0;

    add_required_field("standard_metadata", "ingress_port");
    add_required_field("standard_metadata", "packet_length");
    add_required_field("standard_metadata", "instance_type");
    add_required_field("standard_metadata", "egress_spec");
    add_required_field("standard_metadata", "egress_port");

    force_arith_header("standard_metadata");
    // force_arith_header("queueing_metadata");
    force_arith_header("intrinsic_metadata");

    static int switch_id = 1;
    m_p4SwitchId = switch_id++;
    NS_LOG_INFO("Init P4 Switch with ID: " << m_p4SwitchId);

    m_switchNetDevice = netDevice;
}

P4CorePipeline::~P4CorePipeline()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Destroying P4CorePipeline.");
}

void
P4CorePipeline::InitSwitchWithP4(std::string jsonPath, std::string flowTablePath)
{
    // Log function entry
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Initializing P4CorePipeline.");

    // Initialize status flag and Thrift port
    int status = 0;
    static int p4_switch_ctrl_plane_thrift_port = 9090;
    m_thriftPort = p4_switch_ctrl_plane_thrift_port;

    // Configure OptionsParser
    bm::OptionsParser opt_parser;
    opt_parser.config_file_path = jsonPath;
    opt_parser.debugger_addr = "ipc:///tmp/bmv2-v1model-" +
                               std::to_string(p4_switch_ctrl_plane_thrift_port) + "-debug.ipc";
    opt_parser.notifications_addr = "ipc:///tmp/bmv2-v1model-" +
                                    std::to_string(p4_switch_ctrl_plane_thrift_port) +
                                    "-notifications.ipc";
    opt_parser.file_logger =
        "/tmp/bmv2-v1model-" + std::to_string(p4_switch_ctrl_plane_thrift_port) + "-pipeline.log";
    opt_parser.thrift_port = p4_switch_ctrl_plane_thrift_port++;
    opt_parser.console_logging = false;

    // Initialize the switch
    status = init_from_options_parser(opt_parser);
    if (status != 0)
    {
        NS_LOG_ERROR("Failed to initialize P4CorePipeline.");
        return; // Avoid exiting simulation
    }

    // Start the runtime server
    int port = get_runtime_port();
    bm_runtime::start_server(this, port);

    // Populate flow table using CLI command
    std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string(port) + " < " +
                      flowTablePath + " > /dev/null 2>&1";

    int result = std::system(cmd.c_str());

    // Wait for the server to be ready
    sleep(1);
    if (result != 0)
    {
        NS_LOG_ERROR("Error executing flow table population command: " << cmd);
    }

    NS_LOG_INFO("P4CorePipeline initialization completed successfully.");
}

int
P4CorePipeline::InitFromCommandLineOptions(int argc, char* argv[])
{
    bm::OptionsParser parser;
    parser.parse(argc, argv, m_argParser);

    // create a dummy transport
    std::shared_ptr<bm::TransportIface> transport =
        std::shared_ptr<bm::TransportIface>(bm::TransportIface::make_dummy());

    int status = 0;
    if (parser.no_p4)
        // with out p4-json, acctually the switch will wait for the
        // configuration(p4-json) before work
        status = init_objects_empty(parser.device_id, transport);
    else
        // load p4 configuration files xxxx.json to switch
        status = init_objects(parser.config_file_path, parser.device_id, transport);
    return status;
}

void
P4CorePipeline::RunCli(const std::string& commandsFile)
{
    NS_LOG_FUNCTION(this << " Switch ID: " << m_p4SwitchId << " Running CLI commands from "
                         << commandsFile);

    int port = get_runtime_port();
    bm_runtime::start_server(this, port);
    // start_and_return ();
    NS_LOG_DEBUG("Switch ID: " << m_p4SwitchId << " Waiting for the runtime server to start");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Run the CLI commands to populate table entries
    std::string cmd = "run_bmv2_CLI --thrift_port " + std::to_string(port) + " " + commandsFile;
    int res = std::system(cmd.c_str());
    (void)res;
}

int
P4CorePipeline::receive_(uint32_t port_num, const char* buffer, int len)
{
    NS_LOG_FUNCTION(this << " Switch ID: " << m_p4SwitchId << " Port: " << port_num
                         << " Len: " << len);
    return 0;
}

void
P4CorePipeline::start_and_return_()
{
    NS_LOG_FUNCTION("Switch ID: " << m_p4SwitchId << " start");
}

void
P4CorePipeline::swap_notify_()
{
    NS_LOG_FUNCTION("p4_switch has been notified of a config swap");
}

int
P4CorePipeline::P4ProcessingPipeline(Ptr<Packet> packetIn,
                                     int inPort,
                                     uint16_t protocol,
                                     const Address& destination)
{
    NS_LOG_FUNCTION(this);

    // === Convert ns-3 packet to bm packet
    std::unique_ptr<bm::Packet> bm_packet = ConvertToBmPacket(packetIn, inPort);

    bm::PHV* phv = bm_packet->get_phv();
    uint32_t len = bm_packet.get()->get_data_size();
    bm_packet.get()->set_ingress_port(inPort);

    phv->reset_metadata();
    phv->get_field("standard_metadata.ingress_port").set(inPort);
    bm_packet->set_register(RegisterAccess::PACKET_LENGTH_REG_IDX, len);
    phv->get_field("standard_metadata.packet_length").set(len);
    bm::Field& f_instance_type = phv->get_field("standard_metadata.instance_type");
    f_instance_type.set(PKT_INSTANCE_TYPE_NORMAL);

    // === Parser and MAU processing
    bm::Parser* parser = this->get_parser("parser");
    bm::Pipeline* ingress_mau = this->get_pipeline("ingress");
    parser->parse(bm_packet.get());
    ingress_mau->apply(bm_packet.get());

    bm_packet->reset_exit();
    bm::Field& f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    uint32_t egress_spec = f_egress_spec.get_uint();

    // LEARNING
    int learn_id = RegisterAccess::get_lf_field_list(bm_packet.get());
    if (learn_id > 0)
    {
        get_learn_engine()->learn(learn_id, *bm_packet.get());
    }

    // === Egress
    bm::Pipeline* egress_mau = this->get_pipeline("egress");
    bm::Deparser* deparser = this->get_deparser("deparser");
    phv->get_field("standard_metadata.egress_port").set(egress_spec);
    f_egress_spec = phv->get_field("standard_metadata.egress_spec");
    f_egress_spec.set(0);

    phv->get_field("standard_metadata.packet_length")
        .set(bm_packet->get_register(RegisterAccess::PACKET_LENGTH_REG_IDX));

    egress_mau->apply(bm_packet.get());

    // === Deparser
    deparser->deparse(bm_packet.get());

    // === Send the packet to the destination
    Ptr<Packet> ns_packet = ConvertToNs3Packet(std::move(bm_packet));
    m_switchNetDevice->SendNs3Packet(ns_packet, egress_spec, protocol, destination);
    return 0;
}

Ptr<Packet>
P4CorePipeline::ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bm_packet)
{
    // Create a new ns3::Packet using the data buffer
    char* bm_buf = bm_packet.get()->data();
    size_t len = bm_packet.get()->get_data_size();
    Ptr<Packet> ns_packet = Create<Packet>((uint8_t*)(bm_buf), len);

    return ns_packet;
}

std::unique_ptr<bm::Packet>
P4CorePipeline::ConvertToBmPacket(Ptr<Packet> nsPacket, int inPort)
{
    int len = nsPacket->GetSize();
    uint8_t* pkt_buffer = new uint8_t[len];
    nsPacket->CopyData(pkt_buffer, len);
    bm::PacketBuffer buffer(len + 512, (char*)pkt_buffer, len);
    std::unique_ptr<bm::Packet> bm_packet(
        new_packet_ptr(inPort, m_packetId++, len, std::move(buffer)));
    delete[] pkt_buffer;

    return bm_packet;
}

} // namespace ns3
