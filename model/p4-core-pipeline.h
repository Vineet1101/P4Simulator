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

#ifndef P4_CORE_V1MODEL_PIPELINE_H
#define P4_CORE_V1MODEL_PIPELINE_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-net-device.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>

#define SSWITCH_PIPELINE_DROP_PORT 511

namespace ns3
{

class P4SwitchNetDevice;

/**
 * @brief A P4 Pipeline Implementation to be wrapped in P4 Device
 *
 * The P4CorePipeline uses the pipeline implementation provided by
 * `Behavioral Model` (https://github.com/p4lang/behavioral-model).
 * Internal processing functions and the `switch` class are used.
 * However, P4CorePipeline processes packets in a way adapted to ns-3.
 */
class P4CorePipeline : public bm::Switch
{
  public:
    /**
     * @brief Construct a new P4 Core Pipeline object
     *
     * @param netDevice P4 Switch Net Device
     * @param enableSwap enable swapping in run time
     * @param enableTracing enable tracing
     * @param drop_port set the port to drop packets
     */
    P4CorePipeline(P4SwitchNetDevice* netDevice,
                   bool enableSwap,
                   bool enableTracing,
                   uint32_t drop_port = SSWITCH_PIPELINE_DROP_PORT);
    /**
     * @brief Destroy the P4 Core Pipeline object
     */
    ~P4CorePipeline();

    /**
     * @brief Initialize the switch with P4 program with JSON and flow table
     *
     * @param jsonPath path to the JSON file
     * @param flowTablePath path to the flow table file
     */
    void InitSwitchWithP4(std::string jsonPath, std::string flowTablePath);

    /**
     * @brief Run CLI commands from a file by thrift
     *
     * @param commandsFile path to the commands file
     */
    void RunCli(const std::string& commandsFile);

    /**
     * @brief Initialize the switch with command line options
     */
    int InitFromCommandLineOptions(int argc, char* argv[]);

    /**
     * @brief Receive a packet from the network and process with
     * the P4 pipeline: parser, ingress, egress, deparser, notication:
     * there is no queue buffer in this class
     *
     * @param packetIn the packet passed from the SwitchNetDevice
     * @param inPort the port where the packet is received
     * @param protocol the protocol of the packet
     * @param destination the destination address of the packet
     * @return int 0 if success
     */
    int P4ProcessingPipeline(Ptr<Packet> packetIn,
                             int inPort,
                             uint16_t protocol,
                             const Address& destination);

    /**
     * @brief Convert a bm packet to ns-3 packet
     *
     * @param bmPacket the bm packet
     * @return Ptr<Packet> the ns-3 packet
     */
    Ptr<Packet> ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bmPacket);

    /**
     * @brief Convert a ns-3 packet to bm packet
     *
     * @param nsPacket the ns-3 packet
     * @param inPort the port where the packet is received
     * @return std::unique_ptr<bm::Packet> the bm packet
     */
    std::unique_ptr<bm::Packet> ConvertToBmPacket(Ptr<Packet> nsPacket, int inPort);

    /**
     * @brief [override from bmv2] Receive a packet from the network
     */
    int receive_(uint32_t port_num, const char* buffer, int len) override;

    /**
     * @brief [override from bmv2] Start the switch and return
     */
    void start_and_return_() override;

    /**
     * @brief [override from bmv2] Notify the switch of a config swap
     */
    void swap_notify_() override;

    /**
     * @brief [override from bmv2] Reset the target-specific state
     */
    void reset_target_state_() override;

    P4CorePipeline(const P4CorePipeline&) = delete;
    P4CorePipeline& operator=(const P4CorePipeline&) = delete;
    P4CorePipeline(P4CorePipeline&&) = delete;
    P4CorePipeline&& operator=(P4CorePipeline&&) = delete;

  protected:
    enum PktInstanceType
    {
        PKT_INSTANCE_TYPE_NORMAL, // only normal pkts
    };

  private:
    bool m_enableTracing; //!< Enable tracing
    bool m_enableSwap;    //!< Enable swapping
    int m_p4SwitchId;     //!< ID of the switch
    int m_thriftPort;     //!< Port for thrift server
    uint32_t m_dropPort;  //!< Port to drop packets
    uint64_t m_packetId;  // Packet ID

    P4SwitchNetDevice* m_switchNetDevice; //!< P4 Switch Net Device
    bm::TargetParserBasic* m_argParser;   //!< Structure of parsers
};

} // namespace ns3

#endif // !P4_CORE_V1MODEL_PIPELINE_H