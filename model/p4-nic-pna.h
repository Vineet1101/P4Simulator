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

#ifndef P4_NIC_PNA_H
#define P4_NIC_PNA_H

#include "ns3/p4-queue.h"
#include "ns3/p4-switch-net-device.h"
#include "ns3/packet.h"

#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/queue.h>
#include <bm/bm_sim/queueing.h>
#include <bm/bm_sim/simple_pre_lag.h>
#include <bm/bm_sim/switch.h>
#include <vector>

namespace ns3
{

class P4SwitchNetDevice;

class P4PnaNic : public bm::Switch
{
  public:
    // by default, swapping is off
    explicit P4PnaNic(P4SwitchNetDevice* net_device, bool enable_swap = false);

    ~P4PnaNic();

    int receive_(port_t port_num, const char* buffer, int len) override;

    void start_and_return_() override;

    void reset_target_state_() override;

    bool main_processing_pipeline();

    uint64_t GetTimeStamp();

    Ptr<Packet> ConvertToNs3Packet(std::unique_ptr<bm::Packet>&& bmPacket);

    int GetAddressIndex(const Address& destination);

    void InitSwitchWithP4(std::string jsonPath, std::string flowTablePath);

    // returns the packet id of most recently received packet.
    static uint64_t get_packet_id()
    {
        return (packet_id - 1);
    }

  private:
    static uint64_t packet_id;

    enum PktInstanceType
    {
        FROM_NET_PORT,
        FROM_NET_LOOPEDBACK,
        FROM_NET_RECIRCULATED,
        FROM_HOST,
        FROM_HOST_LOOPEDBACK,
        FROM_HOST_RECIRCULATED,
    };

    enum PktDirection
    {
        NET_TO_HOST,
        HOST_TO_NET,
    };

    int ReceivePacket(Ptr<Packet> packetIn,
                      int inPort,
                      uint16_t protocol,
                      const Address& destination);

  private:
    int m_thriftPort;
    uint64_t m_packetId; // Packet ID
    int m_p4SwitchId;    //!< ID of the switch
    P4SwitchNetDevice* m_switchNetDevice;
    std::vector<Address> m_destinationList; //!< List of addresses (O(log n) search)
    std::map<Address, int> m_addressMap;    //!< Map for fast lookup

    bm::Queue<std::unique_ptr<bm::Packet>> input_buffer;
    uint64_t start_timestamp; //!< Start time of the nic
};

} // namespace ns3

#endif // PNA_NIC_PNA_NIC_H_