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
 */

/*
 * Rupesh Chiluka (rchiluka@marvell.com)
 *
 */

#ifndef P4_NIC_PNA_H
#define P4_NIC_PNA_H

#include "ns3/p4-queue.h"
#include "ns3/packet.h"

#include <bm/bm_sim/queue.h>
#include <bm/bm_sim/queueing.h>
#include <bm/bm_sim/packet.h>
#include <bm/bm_sim/switch.h>
#include <bm/bm_sim/simple_pre_lag.h>

#include <vector>

namespace ns3 {

class P4PnaNic : public bm::Switch
{

public:
  // by default, swapping is off
  explicit P4PnaNic (bool enable_swap = false);

  ~P4PnaNic ();

  int receive_ (port_t port_num, const char *buffer, int len) override;

  void start_and_return_ () override;

  void reset_target_state_ () override;

  uint64_t GetTimeStamp ();

  // returns the packet id of most recently received packet.
  static uint64_t
  get_packet_id ()
  {
    return (packet_id - 1);
  }

private:
  static uint64_t packet_id;

  enum PktInstanceType {
    FROM_NET_PORT,
    FROM_NET_LOOPEDBACK,
    FROM_NET_RECIRCULATED,
    FROM_HOST,
    FROM_HOST_LOOPEDBACK,
    FROM_HOST_RECIRCULATED,
  };

  enum PktDirection {
    NET_TO_HOST,
    HOST_TO_NET,
  };

  int ReceivePacket (Ptr<Packet> packetIn, int inPort, uint16_t protocol,
                     const Address &destination);

private:
  bm::Queue<std::unique_ptr<bm::Packet>> input_buffer;
  uint64_t start_timestamp; //!< Start time of the nic
};

} // namespace ns3

#endif // PNA_NIC_PNA_NIC_H_