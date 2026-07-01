/*
 * Copyright (c) 2025 TU Dresden
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
 * Authors: Vineet Goel <vineetgoel692@gmail.com>
 */

#include "p4-traffic-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("P4TrafficManager");

NS_OBJECT_ENSURE_REGISTERED(P4TrafficManager);

const char*
TmDropReasonToString(TmDropReason r)
{
    switch (r)
    {
    case TmDropReason::VOQ_GLOBAL_BUFFER_FULL:
        return "VOQ_GLOBAL_BUFFER_FULL";
    case TmDropReason::VOQ_INPUT_BUFFER_FULL:
        return "VOQ_INPUT_BUFFER_FULL";
    case TmDropReason::VOQ_QUEUE_FULL:
        return "VOQ_QUEUE_FULL";
    case TmDropReason::EGRESS_PORT_BUFFER_FULL:
        return "EGRESS_PORT_BUFFER_FULL";
    case TmDropReason::EGRESS_QUEUE_FULL:
        return "EGRESS_QUEUE_FULL";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// TmStats helpers
// ---------------------------------------------------------------------------

Time
P4TrafficManager::TmStats::AvgVoqDelay() const
{
    return (cntVoqDelay == 0) ? Time(0) : (sumVoqDelay / static_cast<int64_t>(cntVoqDelay));
}

Time
P4TrafficManager::TmStats::AvgEgressDelay() const
{
    return (cntEgressDelay == 0) ? Time(0) : (sumEgressDelay / static_cast<int64_t>(cntEgressDelay));
}

Time
P4TrafficManager::TmStats::AvgTotalDelay() const
{
    return (cntTotalDelay == 0) ? Time(0) : (sumTotalDelay / static_cast<int64_t>(cntTotalDelay));
}

// ---------------------------------------------------------------------------
// TypeId / construction
// ---------------------------------------------------------------------------

TypeId
P4TrafficManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::P4TrafficManager")
            .SetParent<Object>()
            .SetGroupName("P4sim")
            .AddConstructor<P4TrafficManager>()
            .AddAttribute("NumPorts",
                          "Number of switch ports N (allocates N*N*8 VOQs).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::SetNumPorts,
                                               &P4TrafficManager::GetNumPorts),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("GlobalBufferLimit",
                          "Global buffer limit in bytes across VOQ and egress (0 = unlimited).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::m_globalBufferLimit),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("InputBufferLimit",
                          "Per-input-port buffer limit in bytes (0 = unlimited).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::m_inputBufferLimit),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("VoqLimit",
                          "Per-VOQ[in][out][prio] limit in bytes (0 = unlimited).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::m_voqLimit),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("EgressPortLimit",
                          "Per-output-port egress buffer limit in bytes (0 = unlimited).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::m_egressPortLimit),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("EgressQueueLimit",
                          "Per-egress-queue[out][prio] limit in bytes (0 = unlimited).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&P4TrafficManager::m_egressQueueLimit),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("PortRate",
                          "Output-port serialization rate.",
                          DataRateValue(DataRate("1Gbps")),
                          MakeDataRateAccessor(&P4TrafficManager::m_portRate),
                          MakeDataRateChecker())
            .AddAttribute("FabricRate",
                          "Fabric transfer rate.",
                          DataRateValue(DataRate("10Gbps")),
                          MakeDataRateAccessor(&P4TrafficManager::m_fabricRate),
                          MakeDataRateChecker())
            .AddAttribute("IngressPipelineDelay",
                          "Fixed ingress pipeline processing delay.",
                          TimeValue(Time(0)),
                          MakeTimeAccessor(&P4TrafficManager::m_ingressPipelineDelay),
                          MakeTimeChecker())
            .AddAttribute("FabricArbitrationDelay",
                          "Fixed fabric arbitration delay per round.",
                          TimeValue(Time(0)),
                          MakeTimeAccessor(&P4TrafficManager::m_fabricArbitrationDelay),
                          MakeTimeChecker())
            .AddAttribute("EgressPipelineDelay",
                          "Fixed egress pipeline processing delay.",
                          TimeValue(Time(0)),
                          MakeTimeAccessor(&P4TrafficManager::m_egressPipelineDelay),
                          MakeTimeChecker())
            .AddTraceSource("VoqEnqueue",
                            "A packet was enqueued into a VOQ (in, out, prio, bytes).",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_voqEnqueueTrace),
                            "ns3::P4TrafficManager::QueueOpCallback")
            .AddTraceSource("VoqDequeue",
                            "A packet was dequeued from a VOQ (in, out, prio, bytes).",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_voqDequeueTrace),
                            "ns3::P4TrafficManager::QueueOpCallback")
            .AddTraceSource("EgressEnqueue",
                            "A packet was enqueued into an egress queue (in, out, prio, bytes).",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_egressEnqueueTrace),
                            "ns3::P4TrafficManager::QueueOpCallback")
            .AddTraceSource("EgressDequeue",
                            "A packet was dequeued from an egress queue (in, out, prio, bytes).",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_egressDequeueTrace),
                            "ns3::P4TrafficManager::QueueOpCallback")
            .AddTraceSource("Drop",
                            "A packet was dropped (reason, in, out, prio, bytes).",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_dropTrace),
                            "ns3::P4TrafficManager::DropCallback")
            .AddTraceSource("VoqWaitingDelay",
                            "VOQ waiting delay of a dequeued packet.",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_voqDelayTrace),
                            "ns3::P4TrafficManager::DelayCallback")
            .AddTraceSource("EgressWaitingDelay",
                            "Egress queue waiting delay of a dequeued packet.",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_egressDelayTrace),
                            "ns3::P4TrafficManager::DelayCallback")
            .AddTraceSource("TotalDelay",
                            "Total Traffic Manager delay of a packet.",
                            MakeTraceSourceAccessor(&P4TrafficManager::m_totalDelayTrace),
                            "ns3::P4TrafficManager::DelayCallback");
    return tid;
}

P4TrafficManager::P4TrafficManager()
{
    NS_LOG_FUNCTION(this);
}

P4TrafficManager::~P4TrafficManager()
{
    NS_LOG_FUNCTION(this);
}

void
P4TrafficManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_voq.clear();
    m_voqBytes.clear();
    m_inputBufferBytes.clear();
    m_egressPortBytes.clear();
    m_egressQueueBytes.clear();
    Object::DoDispose();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void
P4TrafficManager::SetNumPorts(uint32_t numPorts)
{
    NS_LOG_FUNCTION(this << numPorts);
    m_numPorts = numPorts;
    AllocateStructures();
}

uint32_t
P4TrafficManager::GetNumPorts() const
{
    return m_numPorts;
}

void
P4TrafficManager::AllocateStructures()
{
    NS_LOG_FUNCTION(this << m_numPorts);

    const uint32_t n = m_numPorts;
    const size_t nn = static_cast<size_t>(n) * n;

    // TmItem is move-only, so the VOQ element type cannot be relocated by
    // vector::resize/assign (those instantiate a copy fallback).  Direct sized
    // construction value-initialises each element in place, which is fine.
    m_voq = std::vector<PriQueues>(nn);
    m_voqBytes = std::vector<PriBytes>(nn, PriBytes{});
    m_inputBufferBytes.assign(n, 0);
    m_egressPortBytes.assign(n, 0);
    m_egressQueueBytes.assign(n, PriBytes{});

    m_globalBufferBytes = 0;
    m_stats.perPortTxBytes.assign(n, 0);

    NS_LOG_INFO("Allocated " << (static_cast<uint64_t>(n) * n * P4_TM_NUM_PRIORITIES)
                             << " VOQs for " << n << " ports");
}

bool
P4TrafficManager::ValidPort(uint32_t p) const
{
    return p < m_numPorts;
}

bool
P4TrafficManager::ValidPriority(uint8_t prio) const
{
    return prio < P4_TM_NUM_PRIORITIES;
}

// ---------------------------------------------------------------------------
// Ingress boundary: EnqueueToVoq
// ---------------------------------------------------------------------------

bool
P4TrafficManager::EnqueueToVoq(std::unique_ptr<TmPayload> payload,
                               uint32_t sizeBytes,
                               uint32_t inPort,
                               uint32_t outPort,
                               uint8_t priority)
{
    NS_LOG_FUNCTION(this << sizeBytes << inPort << outPort << static_cast<uint32_t>(priority));

    NS_ABORT_MSG_IF(!ValidPort(inPort) || !ValidPort(outPort),
                    "EnqueueToVoq: port index out of range (numPorts=" << m_numPorts << ")");
    NS_ABORT_MSG_IF(!ValidPriority(priority), "EnqueueToVoq: priority must be 0..7");

    m_stats.totalReceived++;

    // Admission control, in order: global -> input -> VOQ.
    auto drop = [&](TmDropReason reason) {
        m_stats.totalDropped++;
        m_stats.dropsByReason[static_cast<size_t>(reason)]++;
        m_dropTrace(static_cast<uint8_t>(reason), inPort, outPort, priority, sizeBytes);
        NS_LOG_DEBUG("Dropped " << sizeBytes << "B pkt (" << TmDropReasonToString(reason)
                                << ") in=" << inPort << " out=" << outPort
                                << " prio=" << static_cast<uint32_t>(priority));
        // payload released on return (unique_ptr goes out of scope)
        return false;
    };

    if (m_globalBufferLimit != 0 && m_globalBufferBytes + sizeBytes > m_globalBufferLimit)
    {
        return drop(TmDropReason::VOQ_GLOBAL_BUFFER_FULL);
    }
    if (m_inputBufferLimit != 0 && m_inputBufferBytes[inPort] + sizeBytes > m_inputBufferLimit)
    {
        return drop(TmDropReason::VOQ_INPUT_BUFFER_FULL);
    }
    if (m_voqLimit != 0 && m_voqBytes[Idx(inPort, outPort)][priority] + sizeBytes > m_voqLimit)
    {
        return drop(TmDropReason::VOQ_QUEUE_FULL);
    }

    // Accept.
    TmItem item;
    item.payload = std::move(payload);
    item.sizeBytes = sizeBytes;
    item.inPort = inPort;
    item.outPort = outPort;
    item.priority = priority;
    item.voqEnqueueTime = Simulator::Now();
    item.uid = m_nextUid++;

    m_voq[Idx(inPort, outPort)][priority].push_back(std::move(item));
    m_voqBytes[Idx(inPort, outPort)][priority] += sizeBytes;
    m_inputBufferBytes[inPort] += sizeBytes;
    m_globalBufferBytes += sizeBytes;

    m_stats.totalVoqEnqueued++;
    m_voqEnqueueTrace(inPort, outPort, priority, sizeBytes);

    NS_LOG_DEBUG("Enqueued " << sizeBytes << "B into VOQ[" << inPort << "][" << outPort << "]["
                             << static_cast<uint32_t>(priority) << "]");
    return true;
}

// ---------------------------------------------------------------------------
// Fabric scheduler
// ---------------------------------------------------------------------------

std::vector<Grant>
P4TrafficManager::RunFabricScheduler()
{
    return DoRunFabricScheduler();
}

std::vector<Grant>
P4TrafficManager::DoRunFabricScheduler()
{
    NS_LOG_FUNCTION(this);

    std::vector<Grant> grants;
    if (m_numPorts == 0)
    {
        return grants;
    }

    std::vector<bool> inputUsed(m_numPorts, false);
    std::vector<bool> outputUsed(m_numPorts, false);

    // Priority-first maximal matching: highest priority (7) first.
    for (int p = P4_TM_NUM_PRIORITIES - 1; p >= 0; --p)
    {
        for (uint32_t in = 0; in < m_numPorts; ++in)
        {
            if (inputUsed[in])
            {
                continue;
            }
            for (uint32_t out = 0; out < m_numPorts; ++out)
            {
                if (outputUsed[out])
                {
                    continue;
                }
                if (VoqNotEmpty(in, out, static_cast<uint8_t>(p)))
                {
                    grants.push_back({in, out, static_cast<uint8_t>(p)});
                    inputUsed[in] = true;
                    outputUsed[out] = true;
                    break; // this input is now used; move to next input
                }
            }
        }
    }

    NS_LOG_DEBUG("Fabric scheduler produced " << grants.size() << " grants");
    return grants;
}

bool
P4TrafficManager::DequeueFromVoq(uint32_t inPort,
                                 uint32_t outPort,
                                 uint8_t priority,
                                 TmItem& item)
{
    NS_LOG_FUNCTION(this << inPort << outPort << static_cast<uint32_t>(priority));

    NS_ABORT_MSG_IF(!ValidPort(inPort) || !ValidPort(outPort) || !ValidPriority(priority),
                    "DequeueFromVoq: index out of range");

    auto& q = m_voq[Idx(inPort, outPort)][priority];
    if (q.empty())
    {
        return false;
    }

    item = std::move(q.front());
    q.pop_front();

    const uint32_t sizeBytes = item.sizeBytes;
    m_voqBytes[Idx(inPort, outPort)][priority] -= sizeBytes;
    m_inputBufferBytes[inPort] -= sizeBytes;
    m_globalBufferBytes -= sizeBytes;

    m_stats.totalMovedToEgress++;

    Time voqDelay = Simulator::Now() - item.voqEnqueueTime;
    m_stats.sumVoqDelay += voqDelay;
    m_stats.cntVoqDelay++;
    if (voqDelay > m_stats.maxQueueingDelay)
    {
        m_stats.maxQueueingDelay = voqDelay;
    }

    m_voqDequeueTrace(inPort, outPort, priority, sizeBytes);
    m_voqDelayTrace(voqDelay);

    NS_LOG_DEBUG("Dequeued " << sizeBytes << "B from VOQ[" << inPort << "][" << outPort << "]["
                             << static_cast<uint32_t>(priority) << "] voqDelay="
                             << voqDelay.GetNanoSeconds() << "ns");
    return true;
}

// ---------------------------------------------------------------------------
// Occupancy queries
// ---------------------------------------------------------------------------

size_t
P4TrafficManager::VoqLength(uint32_t inPort, uint32_t outPort, uint8_t priority) const
{
    if (!ValidPort(inPort) || !ValidPort(outPort) || !ValidPriority(priority))
    {
        return 0;
    }
    return m_voq[Idx(inPort, outPort)][priority].size();
}

bool
P4TrafficManager::VoqNotEmpty(uint32_t inPort, uint32_t outPort, uint8_t priority) const
{
    return VoqLength(inPort, outPort, priority) > 0;
}

uint64_t
P4TrafficManager::GlobalBufferBytes() const
{
    return m_globalBufferBytes;
}

uint64_t
P4TrafficManager::InputBufferBytes(uint32_t inPort) const
{
    return ValidPort(inPort) ? m_inputBufferBytes[inPort] : 0;
}

uint64_t
P4TrafficManager::VoqBytes(uint32_t inPort, uint32_t outPort, uint8_t priority) const
{
    if (!ValidPort(inPort) || !ValidPort(outPort) || !ValidPriority(priority))
    {
        return 0;
    }
    return m_voqBytes[Idx(inPort, outPort)][priority];
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

const P4TrafficManager::TmStats&
P4TrafficManager::GetStats() const
{
    return m_stats;
}

void
P4TrafficManager::ResetStats()
{
    const size_t nPorts = m_stats.perPortTxBytes.size();
    m_stats = TmStats{};
    m_stats.perPortTxBytes.assign(nPorts, 0);
}

} // namespace ns3
