/* Copyright 2013-present Barefoot Networks, Inc.
 * Copyright 2021 VMware, Inc.
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
 * Authors: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#ifndef P4_SWITCH_CORE_H
#define P4_SWITCH_CORE_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-net-device.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/switch.h>
#include <map>
#include <vector>

#define SSWITCH_PRIORITY_QUEUEING_SRC "intrinsic_metadata.priority"
#define SSWITCH_OUTPUT_BUFFER_SIZE 1024
#define SSWITCH_DROP_PORT 511
#define SSWITCH_VIRTUAL_QUEUE_NUM 8

namespace ns3
{

class P4SwitchNetDevice;
class InputBuffer;

class P4SwitchCore : public bm::Switch
{
  public:
    // === Constructor & Destructor ===
    P4SwitchCore(P4SwitchNetDevice* netDevice,
                 bool enableSwap,
                 bool enableTracing,
                 port_t dropPort = SSWITCH_DROP_PORT);

    virtual ~P4SwitchCore() = default;

    /**
     * @brief Initialize the switch with the P4 program
     * @param jsonPath the path to the JSON file
     * @return void
     */
    void InitializeSwitchFromP4Json(const std::string& jsonPath);

    /**
     * @brief Load the flow table to the switch
     * @param flowTablePath the path to the flow table file
     * @return int the status code
     */
    int LoadFlowTableToSwitch(const std::string& flowTablePath);

    /**
     * @brief Initialize the switch from command line options
     * @param argc the number of command line arguments
     * @param argv the command line arguments
     * @return int the status code
     */
    int InitFromCommandLineOptions(int argc, char* argv[]);

    /**
     * @brief Execute the CLI commands from a file
     * @param commandsFile the path to the CLI commands file
     * @return int the status code
     */
    int ExecuteCliCommands(const std::string& commandsFile);

    /**
     * @brief Receive a packet from the network
     * @param packetIn the incoming packet
     * @param inPort the switch port where the packet is received
     * @param protocol the protocol of the packet
     * @param destination the destination address of the packet
     * @return int the status code
     */
    virtual int ReceivePacket(Ptr<Packet> packetIn,
                              int inPort,
                              uint16_t protocol,
                              const Address& destination) = 0;

    void SetEgressTimerEvent();

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
     * @brief Returns the elapsed time since the switch started.
     *
     * This function calculates the difference between the current simulation time
     * (in nanoseconds) and the recorded start timestamp of the switch. Notice the
     * p4 only support 48-bit timestamp, so the timestamp will be truncated to 48-bit.
     *
     * @return The elapsed time in nanoseconds as a 64-bit unsigned integer.
     */
    uint64_t GetTimeStamp();

    void PrintSwitchConfig();

    void CalculatePacketsPerSecond();

    // === override ===
    int receive_(uint32_t port_num, const char* buffer, int len) override;
    void start_and_return_() override;
    void swap_notify_() override;
    void reset_target_state_() override;

    // Disabling copy and move operations
    P4SwitchCore(const P4SwitchCore&) = delete;
    P4SwitchCore& operator=(const P4SwitchCore&) = delete;
    P4SwitchCore(P4SwitchCore&&) = delete;
    P4SwitchCore&& operator=(P4SwitchCore&&) = delete;

  protected:
    // Mirroring session configuration
    struct MirroringSessionConfig
    {
        port_t egress_port;
        bool egress_port_valid;
        unsigned int mgid;
        bool mgid_valid;
    };

    /**
     * @brief Add a mirroring session to the switch
     * @param mirrorId the ID of the mirroring session
     * @param config the configuration of the mirroring session
     * @return bool true if the mirroring session is added successfully, false otherwise
     */
    bool AddMirroringSession(int mirrorId, const MirroringSessionConfig& config);

    /**
     * @brief Delete a mirroring session from the switch
     * @param mirrorId the ID of the mirroring session
     * @return bool true if the mirroring session is deleted successfully, false otherwise
     */
    bool DeleteMirroringSession(int mirrorId);

    /**
     * @brief Get the configuration of a mirroring session
     * @param mirrorId the ID of the mirroring session
     * @param config the configuration of the mirroring session
     * @return bool true if the mirroring session is found, false otherwise
     */
    bool GetMirroringSession(int mirrorId, MirroringSessionConfig* config) const;

    struct EgressThreadMapper
    {
        explicit EgressThreadMapper(size_t nb_threads)
            : nb_threads(nb_threads)
        {
        }

        size_t operator()(size_t egress_port) const
        {
            return egress_port % nb_threads;
        }

        size_t nb_threads;
    };

    /**
     * @brief Different types of packet instances
     * @details The packet instance type is used to determine the processing
     * pipeline of the packet. PKT_INSTANCE_TYPE_NORMAL usually will have
     * lower priority than other types.
     */
    enum PktInstanceType
    {
        PKT_INSTANCE_TYPE_NORMAL,
        PKT_INSTANCE_TYPE_INGRESS_CLONE,
        PKT_INSTANCE_TYPE_EGRESS_CLONE,
        PKT_INSTANCE_TYPE_COALESCED,
        PKT_INSTANCE_TYPE_RECIRC,
        PKT_INSTANCE_TYPE_REPLICATION,
        PKT_INSTANCE_TYPE_RESUBMIT,
    };

    /**
     * @brief Check the queueing metadata
     */
    void CheckQueueingMetadata();

    /**
     * @brief Ingress processing pipeline
     */
    virtual void HandleIngressPipeline() = 0;

    void Enqueue(port_t egress_port, std::unique_ptr<bm::Packet>&& packet);

    /**
     * @brief Egress processing pipeline
     * @param workerId the worker ID
     * @return bool true if the packet is processed successfully, false otherwise
     */
    virtual bool HandleEgressPipeline(size_t workerId) = 0;

    /**
     * @brief Retrieves the index of the given destination address.
     *
     * This function checks if the specified destination address already exists
     * in the internal address map. If it exists, the corresponding index is returned.
     * If not, a new index is assigned by adding the address to the destination list,
     * updating the map, and then returning the newly assigned index.
     *
     * @param destination The destination address to look up.
     * @return The index of the destination address in the internal list.
     *
     * @note This function updates the internal address map and destination list
     *       if the address is not found, ensuring a unique index for each address.
     */
    int GetAddressIndex(const Address& destination);

  private:
    class MirroringSessions;
    P4SwitchNetDevice* m_switchNetDevice;

    bool m_enableTracing;
    uint64_t m_packetId; // Packet ID

    int m_p4SwitchId; //!< ID of the switch
    int m_thriftPort;

    std::string m_thriftCommand;

    port_t m_dropPort; //!< Port to drop packets
    size_t m_nbQueuesPerPort;
    bool m_enableQueueingMetadata{true};
    static constexpr size_t m_nbEgressThreads = 1u; // 4u default in bmv2

    uint64_t m_startTimestamp; //!< Start time of the switch

    // Miscellaneous
    bm::TargetParserBasic* m_argParser;     //!< Structure of parsers
    std::vector<Address> m_destinationList; //!< List of addresses (O(log n) search)
    std::map<Address, int> m_addressMap;    //!< Map for fast lookup
    std::shared_ptr<bm::McSimplePreLAG> m_pre;
    std::unique_ptr<MirroringSessions> m_mirroringSessions;
};

} // namespace ns3

#endif // !P4_CORE_V1MODEL_H