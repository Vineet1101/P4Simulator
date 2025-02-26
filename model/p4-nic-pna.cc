/* Copyright 2024 Marvell Technology, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Rupesh Chiluka <rchiluka@marvell.com>
 * Modified: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#include "ns3/log.h"
#include "ns3/p4-nic-pna.h"
#include "ns3/register_access.h"
#include "ns3/simulator.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#include <bm/spdlog/spdlog.h>
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_DEBUG

#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/options_parse.h>
#include <bm/bm_sim/parser.h>
#include <bm/bm_sim/tables.h>

NS_LOG_COMPONENT_DEFINE("P4CorePna");

namespace ns3
{

uint64_t P4PnaNic::packet_id = 0;

P4PnaNic::P4PnaNic(P4SwitchNetDevice* net_device, bool enable_swap)
    : bm::Switch(enable_swap),
      input_buffer(1024),
      start_timestamp(Simulator::Now().GetNanoSeconds())
{
    add_required_field("pna_main_parser_input_metadata", "recirculated");
    add_required_field("pna_main_parser_input_metadata", "input_port");

    add_required_field("pna_main_input_metadata", "recirculated");
    add_required_field("pna_main_input_metadata", "timestamp");
    add_required_field("pna_main_input_metadata", "parser_error");
    add_required_field("pna_main_input_metadata", "class_of_service");
    add_required_field("pna_main_input_metadata", "input_port");

    add_required_field("pna_main_output_metadata", "class_of_service");

    force_arith_header("pna_main_parser_input_metadata");
    force_arith_header("pna_main_input_metadata");
    force_arith_header("pna_main_output_metadata");

    m_packetId = 0;

    static int switch_id = 1;
    m_p4SwitchId = switch_id++;
    NS_LOG_INFO("Init P4 Switch with ID: " << m_p4SwitchId);

    m_switchNetDevice = net_device;
}

P4PnaNic::~P4PnaNic()
{
    input_buffer.push_front(nullptr);
}

void
P4PnaNic::reset_target_state_()
{
    NS_LOG_FUNCTION(this);
    get_component<bm::McSimplePreLAG>()->reset_state();
}

uint64_t
P4PnaNic::GetTimeStamp()
{
    return Simulator::Now().GetNanoSeconds() - start_timestamp;
}

bool
P4PnaNic::main_processing_pipeline()
{
    NS_LOG_FUNCTION(this);

    std::unique_ptr<bm::Packet> bm_packet;
    input_buffer.pop_back(&bm_packet);
    if (bm_packet == nullptr)
        return false;

    bm::PHV* phv = bm_packet->get_phv();
    auto input_port = phv->get_field("pna_main_parser_input_metadata.input_port").get_uint();

    NS_LOG_DEBUG("Processing packet received on port " << input_port);

    phv->get_field("pna_main_input_metadata.timestamp").set(GetTimeStamp());

    bm::Parser* parser = this->get_parser("main_parser");
    parser->parse(bm_packet.get());

    // pass relevant values from main parser
    phv->get_field("pna_main_input_metadata.recirculated")
        .set(phv->get_field("pna_main_parser_input_metadata.recirculated"));
    phv->get_field("pna_main_input_metadata.parser_error").set(bm_packet->get_error_code().get());
    phv->get_field("pna_main_input_metadata.class_of_service").set(0);
    phv->get_field("pna_main_input_metadata.input_port")
        .set(phv->get_field("pna_main_parser_input_metadata.input_port"));

    bm::Pipeline* main_mau = this->get_pipeline("main_control");
    main_mau->apply(bm_packet.get());
    bm_packet->reset_exit();

    bm::Deparser* deparser = this->get_deparser("main_deparser");
    deparser->deparse(bm_packet.get());

    int port = bm_packet->get_egress_port();
    uint16_t protocol = RegisterAccess::get_ns_protocol(bm_packet.get());
    int addr_index = RegisterAccess::get_ns_address(bm_packet.get());

    Ptr<Packet> ns_packet = this->ConvertToNs3Packet(std::move(bm_packet));
    m_switchNetDevice->SendNs3Packet(ns_packet, port, protocol, m_destinationList[addr_index]);
    return true;
}

int
P4PnaNic::ReceivePacket(Ptr<Packet> packetIn,
                        int inPort,
                        uint16_t protocol,
                        const Address& destination)
{
    int len = packetIn->GetSize();
    uint8_t* pkt_buffer = new uint8_t[len];
    packetIn->CopyData(pkt_buffer, len);
    // we limit the packet buffer to original size + 512 bytes, which means we
    // cannot add more than 512 bytes of header data to the packet, which should
    // be more than enough
    bm::PacketBuffer buffer(len + 512, (char*)pkt_buffer, len);
    std::unique_ptr<bm::Packet> bm_packet =
        new_packet_ptr(inPort, packet_id++, len, std::move(buffer));
    delete[] pkt_buffer;

    bm::PHV* phv = bm_packet->get_phv();

    // many current p4 programs assume this
    // from pna spec - PNA does not mandate initialization of user-defined
    // metadata to known values as given as input to the parser
    phv->reset_metadata();

    // setting ns3 specific metadata in packet register
    RegisterAccess::clear_all(bm_packet.get());
    RegisterAccess::set_ns_protocol(bm_packet.get(), protocol);
    int addr_index = GetAddressIndex(destination);
    RegisterAccess::set_ns_address(bm_packet.get(), addr_index);

    phv->get_field("pna_main_parser_input_metadata.recirculated").set(0);
    phv->get_field("pna_main_parser_input_metadata.input_port").set(inPort);

    // using packet register 0 to store length, this register will be updated for
    // each add_header / remove_header primitive call
    bm_packet->set_register(0, len);

    input_buffer.push_front(std::move(bm_packet));
    return 0;
}

int
P4PnaNic::receive_(port_t port_num, const char* buffer, int len)
{
    NS_LOG_FUNCTION(this << " Port: " << port_num << " Len: " << len);
    return 0;
}

void
P4PnaNic::start_and_return_()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Packet>
P4PnaNic::ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bm_packet)
{
    // Create a new ns3::Packet using the data buffer
    char* bm_buf = bm_packet.get()->data();
    size_t len = bm_packet.get()->get_data_size();
    Ptr<Packet> ns_packet = Create<Packet>((uint8_t*)(bm_buf), len);

    return ns_packet;
}

int
P4PnaNic::GetAddressIndex(const Address& destination)
{
    auto it = m_addressMap.find(destination);
    if (it != m_addressMap.end())
    {
        return it->second;
    }
    else
    {
        int new_index = m_destinationList.size();
        m_destinationList.push_back(destination);
        m_addressMap[destination] = new_index;
        return new_index;
    }
}

void
P4PnaNic::InitSwitchWithP4(std::string jsonPath, std::string flowTablePath)
{
    NS_LOG_FUNCTION(this); // Log function entry

    int status = 0; // Status flag for initialization

    /**
     * @brief NS3PIFOTM mode initializes the switch using a JSON file in jsonPath
     * and populates the flow table entry in flowTablePath.
     */
    NS_LOG_INFO("Initializing p4 PNA with options parser.");

    static int p4_switch_ctrl_plane_thrift_port = 9090;
    m_thriftPort = p4_switch_ctrl_plane_thrift_port;

    bm::OptionsParser opt_parser;

    opt_parser.config_file_path = jsonPath;
    opt_parser.debugger_addr =
        "ipc:///tmp/bmv2-pna-" + std::to_string(p4_switch_ctrl_plane_thrift_port) + "-debug.ipc";
    opt_parser.notifications_addr = "ipc:///tmp/bmv2-pna-" +
                                    std::to_string(p4_switch_ctrl_plane_thrift_port) +
                                    "-notifications.ipc";
    opt_parser.file_logger =
        "/tmp/bmv2-pna-" + std::to_string(p4_switch_ctrl_plane_thrift_port) + "-pipeline.log";
    opt_parser.thrift_port = p4_switch_ctrl_plane_thrift_port++;
    opt_parser.console_logging = false;

    status = 0;
    status = init_from_options_parser(opt_parser);
    if (status != 0)
    {
        NS_LOG_ERROR("Failed to initialize p4 PNA with options parser.");
        return; // Avoid exiting simulation
    }

    if (flowTablePath != "")
    {
        NS_LOG_WARN("Flow table path now is not supported in PNA ARCH.");
        // // Start the runtime server
        // int port = get_runtime_port();
        // bm_runtime::start_server(this, port);

        // // Execute CLI command to populate the flow table
        // std::string cmd = "psa_switch_CLI --thrift-port " + std::to_string(port) + " < " +
        //                   flowTablePath + " > /dev/null 2>&1";
        // int result = std::system(cmd.c_str());

        // // sleep for 2 second to avoid the server not ready
        // sleep(2);

        // if (result != 0)
        // {
        //     NS_LOG_ERROR("Error executing flow table population command: " << cmd);
        // }
    }

    NS_LOG_INFO("P4 PNA initialization completed successfully.");
}

} // namespace ns3
