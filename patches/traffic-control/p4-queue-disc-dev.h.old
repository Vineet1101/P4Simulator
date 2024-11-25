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

#ifndef P4_QUEUE_DISC_H
#define P4_QUEUE_DISC_H

#include "queue-disc.h"

#include <array>

namespace ns3
{

/// Priority map
typedef std::array<uint16_t, 16> Priomap;

/**
 * \ingroup traffic-control
 *
 * The Prio qdisc is a simple classful queueing discipline that contains an
 * arbitrary number of classes of differing priority. The classes are dequeued
 * in numerical descending order of priority. By default, three Fifo queue
 * discs are created, unless the user provides (at least two) child queue
 * discs.
 *
 * If no packet filter is installed or able to classify a packet, then the
 * packet is assigned a priority band based on its priority (modulo 16), which
 * is used as an index into an array called priomap. If a packet is classified
 * by a packet filter and the returned value is non-negative and less than the
 * number of priority bands, then the packet is assigned the priority band
 * corresponding to the value returned by the packet filter. Otherwise, the
 * packet is assigned the priority band specified by the first element of the
 * priomap array.
 * 
 * 
 * \attention [Taken from the original bmv2 QueueingLogicPriRL code]
 * This class is slightly more advanced than QueueingLogicRL. The difference
 * between the 2 is that this one offers the ability to set several priority
 * queues for each logical queue. Priority queues are numbered from `0` to
 * `nb_priorities` (see NSQueueingLogicPriRL::NSQueueingLogicPriRL()). Priority `0`
 * is the highest priority queue. Each priority queue can have its own rate and
 * its own capacity. Queues will be served in order of priority, until their
 * respective maximum rate is reached. If no maximum rate is set, queues with a
 * high priority can starve lower-priority queues. For example, if the queue
 * with priority `0` always contains at least one element, the other queues
 * will never be served.
 * As for QueueingLogicRL, the write behavior (push_front()) is not blocking:
 * once a logical queue is full, subsequent incoming elements will be dropped
 * until the queue starts draining again.
 * Look at the documentation for QueueingLogic for more information about the
 * template parameters (they are the same).
 */
class PrioQueueDisc : public QueueDisc
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * \brief PrioQueueDisc constructor
     */
    PrioQueueDisc();

    ~PrioQueueDisc() override;

    /**
     * Set the band (class) assigned to packets with specified priority.
     *
     * \param prio the priority of packets (with 3 bits, a value between 0 and 8).
     * \param band the band assigned to packets.
     */
    void SetBandForPriority(uint8_t prio, uint16_t band);

    /**
     * Get the band (class) assigned to packets with specified priority.
     *
     * \param prio the priority of packets (with 3 bits, a value between 0 and 8).
     * \returns the band assigned to packets.
     */
    uint16_t GetBandForPriority(uint8_t prio) const;

  private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;
    void InitializeParams() override;

    Priomap m_prio2band; //!< Priority to band mapping
};

/**
 * Serialize the priomap to the given ostream
 *
 * \param os
 * \param priomap
 *
 * \return std::ostream
 */
std::ostream& operator<<(std::ostream& os, const Priomap& priomap);

/**
 * Serialize from the given istream to this priomap.
 *
 * \param is
 * \param priomap
 *
 * \return std::istream
 */
std::istream& operator>>(std::istream& is, Priomap& priomap);

ATTRIBUTE_HELPER_HEADER(Priomap);

} // namespace ns3

#endif /* P4_QUEUE_DISC_H */
