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
 * Authors: Mingyu Ma <mingyu.ma@tu-dresden.de>
 */

#ifndef P4_INPUT_QUEUE_BUFFER_H
#define P4_INPUT_QUEUE_BUFFER_H

#include "queue-disc.h"

#include <array>

namespace ns3
{

/**
 * \ingroup traffic-control
 *
 * The Prio qdisc is a simple classful queueing discipline that contains two
 * classes of differing priority. The function checks the type of the packet
 * based on PacketType.
 * - NORMAL packets are enqueued in the low-priority queue.
 * - RESUBMIT, RECIRCULATE, and other higher-priority packets are enqueued
 * in the high-priority queue.
 * The default priority of the packet is set to NORMAL.
 */

class P4InputQueueBufferDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief P4InputQueueBufferDisc constructor
     */
    P4InputQueueBufferDisc();

    ~P4InputQueueBufferDisc() override;

    enum class PacketType
    {
        NORMAL,
        RESUBMIT,
        RECIRCULATE,
        SENTINEL // signal for the ingress thread to terminate
    };

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    bool DoEnqueue(Ptr<QueueDiscItem> item, PacketType packet_type);
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;
};

} // namespace ns3

#endif /* P4_INPUT_QUEUE_BUFFER_H */
