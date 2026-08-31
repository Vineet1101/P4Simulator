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

#ifndef P4_TRAFFIC_MANAGER_H
#define P4_TRAFFIC_MANAGER_H

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <array>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace ns3
{

/**
 * \ingroup p4sim
 *
 * \brief High-fidelity switch Traffic Manager (VOQ + fabric arbitration).
 *
 * This is the first stage of a high-fidelity traffic-manager architecture that
 * replaces the output-only priority scheduler (NSQueueingLogicPriRL).  The
 * intended packet flow is:
 *
 *   ingress pipeline -> EnqueueToVoq(VOQ[in][out][prio])
 *       -> fabric scheduler grants (in -> out)
 *       -> egress queue (per output port / priority)
 *       -> egress pipeline -> ns-3 NetDevice
 *
 * This file implements phases P1 (VOQ + finite-buffer accounting + drop
 * reasons + stats/traces) and P2 (priority-first maximal-matching fabric
 * scheduler).  The event-driven timing model and the egress side are added in
 * later phases.
 *
 * Packet-format boundary: the Traffic Manager does NOT assume ns3::Packet.  It
 * carries an opaque, move-only ::ns3::TmPayload (see BmPacketPayload for the
 * bm::Packet wrapper used by the real switch core).  Scheduling and accounting
 * are driven solely by the metadata in TmItem, never by the payload contents.
 */

/// Number of priority levels.  The priority field is 3 bits, so 8 levels.
/// Higher value = higher priority (priority 7 is the highest).
static constexpr uint8_t P4_TM_NUM_PRIORITIES = 8;

/**
 * \brief Reason a packet was dropped by the Traffic Manager.
 */
enum class TmDropReason : uint8_t
{
    VOQ_GLOBAL_BUFFER_FULL = 0, ///< global buffer would overflow
    VOQ_INPUT_BUFFER_FULL,      ///< per-input-port buffer would overflow
    VOQ_QUEUE_FULL,             ///< the target VOQ[in][out][prio] would overflow
    EGRESS_PORT_BUFFER_FULL,    ///< per-output-port egress buffer would overflow
    EGRESS_QUEUE_FULL,          ///< the target egress queue[out][prio] would overflow
};

/// Human-readable name for a drop reason (for logging/tracing).
const char* TmDropReasonToString(TmDropReason r);

/**
 * \brief Opaque, move-only payload carried by the Traffic Manager.
 *
 * The TM never inspects the payload; it only moves it from VOQ to egress.
 * Concrete payloads (a bm::Packet wrapper, or a unit-test stub) derive from
 * this base.  This keeps the TM decoupled from any specific packet format.
 */
class TmPayload
{
  public:
    virtual ~TmPayload() = default;
};

/**
 * \brief A unit of work inside the Traffic Manager.
 *
 * Packet-level only: no cell segmentation.  Move-only, because it owns the
 * payload.
 */
struct TmItem
{
    std::unique_ptr<TmPayload> payload; ///< opaque payload (bm::Packet, test stub, ...)
    uint32_t sizeBytes{0};              ///< size in bytes, used for buffer accounting
    uint32_t inPort{0};                 ///< ingress port
    uint32_t outPort{0};                ///< egress port chosen by ingress
    uint8_t priority{0};                ///< 0..7, 7 = highest
    Time voqEnqueueTime;                ///< when the item entered the VOQ
    Time egressEnqueueTime;             ///< when the item entered the egress queue
    uint64_t uid{0};                    ///< monotonically increasing id (tracing / tie-break)

    TmItem() = default;
    TmItem(TmItem&&) = default;
    TmItem& operator=(TmItem&&) = default;
    TmItem(const TmItem&) = delete;
    TmItem& operator=(const TmItem&) = delete;
};

/**
 * \brief A fabric matching decision produced by the scheduler.
 *
 * Instructs the fabric to move one packet from VOQ[inPort][outPort][priority]
 * to the egress side.
 */
struct Grant
{
    uint32_t inPort;
    uint32_t outPort;
    uint8_t priority;
};

/**
 * \ingroup p4sim
 * \brief Traffic Manager: input-side VOQ + priority-first fabric scheduler.
 */
class P4TrafficManager : public Object
{
  public:
    /// TracedCallback signature for VOQ/egress enqueue and dequeue events.
    typedef void (*QueueOpCallback)(uint32_t inPort,
                                    uint32_t outPort,
                                    uint8_t priority,
                                    uint32_t sizeBytes);
    /// TracedCallback signature for drop events.
    typedef void (*DropCallback)(uint8_t reason,
                                 uint32_t inPort,
                                 uint32_t outPort,
                                 uint8_t priority,
                                 uint32_t sizeBytes);
    /// TracedCallback signature for waiting-delay measurements.
    typedef void (*DelayCallback)(Time delay);

    static TypeId GetTypeId();

    P4TrafficManager();
    ~P4TrafficManager() override;

    // ---- Setup ----

    /**
     * \brief Set the number of switch ports and (re)allocate all structures.
     *
     * Allocates N*N*8 VOQs and the associated byte counters.  Also settable
     * via the "NumPorts" attribute.
     * \param numPorts number of ports N.
     */
    void SetNumPorts(uint32_t numPorts);
    uint32_t GetNumPorts() const;

    // ---- Ingress boundary ----

    /**
     * \brief Enqueue a packet into VOQ[inPort][outPort][priority].
     *
     * Finite-buffer admission is checked in order: global, input, VOQ.  If any
     * limit would be exceeded the packet is dropped, m_dropTrace fires with the
     * corresponding TmDropReason, and the function returns false (payload is
     * released).
     *
     * \param payload   the opaque payload (ownership taken on success).
     * \param sizeBytes  packet size in bytes (for accounting).
     * \param inPort    ingress port (< numPorts).
     * \param outPort   egress port (< numPorts).
     * \param priority  priority 0..7 (7 = highest).
     * \return true if accepted into the VOQ, false if dropped.
     */
    bool EnqueueToVoq(std::unique_ptr<TmPayload> payload,
                      uint32_t sizeBytes,
                      uint32_t inPort,
                      uint32_t outPort,
                      uint8_t priority);

    // ---- Fabric ----

    /**
     * \brief Run one round of fabric matching.
     *
     * Delegates to DoRunFabricScheduler() (overridable to swap in iSLIP /
     * round-robin).  The default policy is priority-first maximal matching.
     * \return the list of grants for this round.
     */
    std::vector<Grant> RunFabricScheduler();

    /**
     * \brief Remove the head item of VOQ[inPort][outPort][priority].
     *
     * Used to apply a Grant.  Updates all byte counters and fires the VOQ
     * dequeue / VOQ-waiting-delay traces.
     * \param inPort   ingress port.
     * \param outPort  egress port.
     * \param priority priority level.
     * \param[out] item  receives the dequeued item on success.
     * \return true if an item was dequeued, false if the VOQ was empty.
     */
    bool DequeueFromVoq(uint32_t inPort, uint32_t outPort, uint8_t priority, TmItem& item);

    // ---- Occupancy queries ----

    /// Number of packets queued in VOQ[in][out][prio].
    size_t VoqLength(uint32_t inPort, uint32_t outPort, uint8_t priority) const;
    /// True if VOQ[in][out][prio] holds at least one packet.
    bool VoqNotEmpty(uint32_t inPort, uint32_t outPort, uint8_t priority) const;
    /// Total bytes occupied across VOQ (and, later, egress).
    uint64_t GlobalBufferBytes() const;
    /// Bytes occupied by all VOQs of a given input port.
    uint64_t InputBufferBytes(uint32_t inPort) const;
    /// Bytes occupied by VOQ[in][out][prio].
    uint64_t VoqBytes(uint32_t inPort, uint32_t outPort, uint8_t priority) const;

    // ---- Statistics ----

    /**
     * \brief Cumulative Traffic Manager statistics.
     */
    struct TmStats
    {
        uint64_t totalReceived{0};       ///< packets offered to EnqueueToVoq
        uint64_t totalVoqEnqueued{0};    ///< packets accepted into a VOQ
        uint64_t totalMovedToEgress{0};  ///< packets dequeued from VOQ (granted)
        uint64_t totalTransmitted{0};    ///< packets handed to the wire (later phases)
        uint64_t totalDropped{0};        ///< packets dropped for any reason
        std::array<uint64_t, 5> dropsByReason{}; ///< indexed by TmDropReason
        std::array<uint64_t, P4_TM_NUM_PRIORITIES> perPriorityTransmitted{};

        // delay accumulators (sums + counts -> averages via helpers)
        Time sumVoqDelay;
        uint64_t cntVoqDelay{0};
        Time sumEgressDelay;
        uint64_t cntEgressDelay{0};
        Time sumTotalDelay;
        uint64_t cntTotalDelay{0};
        Time maxQueueingDelay;

        std::vector<uint64_t> perPortTxBytes; ///< bytes transmitted per output port

        Time AvgVoqDelay() const;
        Time AvgEgressDelay() const;
        Time AvgTotalDelay() const;
    };

    const TmStats& GetStats() const;
    void ResetStats();

  protected:
    void DoDispose() override;

    /**
     * \brief The fabric matching policy.
     *
     * Default: priority-first maximal matching.  For priority 7 down to 0, for
     * each unused input port, grant the first unused output port that has a
     * non-empty VOQ at that priority.  One input grants at most one packet and
     * one output receives at most one packet per round.
     *
     * Override this method (or replace the whole object) to implement iSLIP,
     * round-robin, etc.  It only needs VoqNotEmpty() as its primitive.
     * \return the grants for this round.
     */
    virtual std::vector<Grant> DoRunFabricScheduler();

  private:
    /// (Re)allocate VOQ and accounting structures for m_numPorts ports.
    void AllocateStructures();
    /// True if idx < m_numPorts and priority < 8.
    bool ValidPort(uint32_t p) const;
    bool ValidPriority(uint8_t prio) const;
    /// Flatten (inPort, outPort) into the [in*N + out] VOQ index.
    size_t Idx(uint32_t inPort, uint32_t outPort) const
    {
        return static_cast<size_t>(inPort) * m_numPorts + outPort;
    }

    uint32_t m_numPorts{0};

    // Finite-buffer limits (bytes).  0 means "no limit".
    uint64_t m_globalBufferLimit{0};
    uint64_t m_inputBufferLimit{0};
    uint64_t m_voqLimit{0};
    uint64_t m_egressPortLimit{0};
    uint64_t m_egressQueueLimit{0};

    // Timing configuration (used by later phases; part of the config surface).
    DataRate m_portRate;
    DataRate m_fabricRate;
    Time m_ingressPipelineDelay;
    Time m_fabricArbitrationDelay;
    Time m_egressPipelineDelay;

    // ---- VOQ storage: logically queues[inPort][outPort][priority] ----
    // Stored flat (size N*N, indexed by Idx(in,out)) because the element type
    // holds move-only TmItem and cannot be relocated by vector::resize/assign.
    using PriQueues = std::array<std::deque<TmItem>, P4_TM_NUM_PRIORITIES>;
    std::vector<PriQueues> m_voq; ///< [Idx(in,out)] -> 8 priority deques

    // ---- Byte accounting ----
    uint64_t m_globalBufferBytes{0};
    std::vector<uint64_t> m_inputBufferBytes; ///< [in]
    using PriBytes = std::array<uint64_t, P4_TM_NUM_PRIORITIES>;
    std::vector<PriBytes> m_voqBytes;         ///< [Idx(in,out)] -> 8 counters
    std::vector<uint64_t> m_egressPortBytes;  ///< [out] (reserved for P4)
    std::vector<PriBytes> m_egressQueueBytes; ///< [out] -> 8 counters (reserved for P4)

    uint64_t m_nextUid{0};
    TmStats m_stats;

    // ---- Trace sources ----
    TracedCallback<uint32_t, uint32_t, uint8_t, uint32_t> m_voqEnqueueTrace;
    TracedCallback<uint32_t, uint32_t, uint8_t, uint32_t> m_voqDequeueTrace;
    TracedCallback<uint32_t, uint32_t, uint8_t, uint32_t> m_egressEnqueueTrace;
    TracedCallback<uint32_t, uint32_t, uint8_t, uint32_t> m_egressDequeueTrace;
    TracedCallback<uint8_t, uint32_t, uint32_t, uint8_t, uint32_t> m_dropTrace;
    TracedCallback<Time> m_voqDelayTrace;
    TracedCallback<Time> m_egressDelayTrace;
    TracedCallback<Time> m_totalDelayTrace;
};

// Note: the bm::Packet payload wrapper (BmPacketPayload : public TmPayload)
// is introduced in the P4CoreV1model integration phase.  Keeping the TM free
// of bm headers here preserves the packet-format boundary and lets the unit
// tests exercise it with a lightweight TmPayload stub.

} // namespace ns3

#endif /* P4_TRAFFIC_MANAGER_H */
