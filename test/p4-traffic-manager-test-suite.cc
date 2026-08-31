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

#include "ns3/p4-traffic-manager.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <map>

using namespace ns3;

namespace
{

/// Lightweight test payload so the TM can be exercised without a bm::Packet.
class DummyPayload : public TmPayload
{
  public:
    explicit DummyPayload(uint32_t tag)
        : m_tag(tag)
    {
    }

    uint32_t GetTag() const
    {
        return m_tag;
    }

  private:
    uint32_t m_tag;
};

/// Convenience: make a payload and enqueue it into the VOQ.
bool
Enq(Ptr<P4TrafficManager> tm,
    uint32_t size,
    uint32_t in,
    uint32_t out,
    uint8_t prio,
    uint32_t tag = 0)
{
    return tm->EnqueueToVoq(std::make_unique<DummyPayload>(tag), size, in, out, prio);
}

Ptr<P4TrafficManager>
MakeTm(uint32_t numPorts)
{
    Ptr<P4TrafficManager> tm = CreateObject<P4TrafficManager>();
    tm->SetNumPorts(numPorts);
    return tm;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. Basic enqueue / dequeue correctness + VOQ sizing
// ---------------------------------------------------------------------------
class TmBasicEnqueueDequeueTest : public TestCase
{
  public:
    TmBasicEnqueueDequeueTest()
        : TestCase("TM: basic enqueue/dequeue and accounting")
    {
    }

    void DoRun() override
    {
        Ptr<P4TrafficManager> tm = MakeTm(4);

        NS_TEST_ASSERT_MSG_EQ(tm->GetNumPorts(), 4, "NumPorts should be 4");
        NS_TEST_ASSERT_MSG_EQ(tm->GlobalBufferBytes(), 0, "global bytes start at 0");

        NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 1, 3, 42), true, "enqueue should succeed");
        NS_TEST_ASSERT_MSG_EQ(tm->VoqLength(0, 1, 3), 1, "VOQ[0][1][3] len == 1");
        NS_TEST_ASSERT_MSG_EQ(tm->VoqBytes(0, 1, 3), 100, "VOQ bytes == 100");
        NS_TEST_ASSERT_MSG_EQ(tm->InputBufferBytes(0), 100, "input bytes == 100");
        NS_TEST_ASSERT_MSG_EQ(tm->GlobalBufferBytes(), 100, "global bytes == 100");

        // Second packet, same VOQ.
        NS_TEST_ASSERT_MSG_EQ(Enq(tm, 200, 0, 1, 3, 43), true, "enqueue 2 should succeed");
        NS_TEST_ASSERT_MSG_EQ(tm->VoqLength(0, 1, 3), 2, "VOQ len == 2");
        NS_TEST_ASSERT_MSG_EQ(tm->GlobalBufferBytes(), 300, "global bytes == 300");

        // FIFO order within a VOQ: first out is tag 42.
        TmItem item;
        NS_TEST_ASSERT_MSG_EQ(tm->DequeueFromVoq(0, 1, 3, item), true, "dequeue should succeed");
        auto* p = dynamic_cast<DummyPayload*>(item.payload.get());
        NS_TEST_ASSERT_MSG_NE(p, nullptr, "payload must survive as DummyPayload");
        NS_TEST_ASSERT_MSG_EQ(p->GetTag(), 42, "FIFO: first dequeued tag == 42");
        NS_TEST_ASSERT_MSG_EQ(tm->VoqBytes(0, 1, 3), 200, "VOQ bytes back to 200");
        NS_TEST_ASSERT_MSG_EQ(tm->GlobalBufferBytes(), 200, "global bytes == 200");

        // Dequeue empty VOQ returns false.
        NS_TEST_ASSERT_MSG_EQ(tm->DequeueFromVoq(2, 2, 0, item), false, "empty VOQ dequeue false");

        const auto& s = tm->GetStats();
        NS_TEST_ASSERT_MSG_EQ(s.totalReceived, 2, "received == 2");
        NS_TEST_ASSERT_MSG_EQ(s.totalVoqEnqueued, 2, "enqueued == 2");
        NS_TEST_ASSERT_MSG_EQ(s.totalMovedToEgress, 1, "moved to egress == 1");
        NS_TEST_ASSERT_MSG_EQ(s.totalDropped, 0, "no drops");

        Simulator::Destroy();
    }
};

// ---------------------------------------------------------------------------
// 2. Priority scheduling correctness (fabric picks highest priority first)
// ---------------------------------------------------------------------------
class TmPrioritySchedulingTest : public TestCase
{
  public:
    TmPrioritySchedulingTest()
        : TestCase("TM: fabric serves higher priority first")
    {
    }

    void DoRun() override
    {
        Ptr<P4TrafficManager> tm = MakeTm(2);

        // Same input->output, different priorities: 2 and 5.
        Enq(tm, 100, 0, 1, 2, 1);
        Enq(tm, 100, 0, 1, 5, 2);

        auto grants = tm->RunFabricScheduler();
        NS_TEST_ASSERT_MSG_EQ(grants.size(), 1, "one input -> one grant per round");
        NS_TEST_ASSERT_MSG_EQ(static_cast<uint32_t>(grants[0].priority), 5,
                              "priority 5 must be served before 2");
        NS_TEST_ASSERT_MSG_EQ(grants[0].inPort, 0, "granted input 0");
        NS_TEST_ASSERT_MSG_EQ(grants[0].outPort, 1, "granted output 1");

        Simulator::Destroy();
    }
};

// ---------------------------------------------------------------------------
// 3. VOQ correctness: two inputs target the same output
// ---------------------------------------------------------------------------
class TmVoqSameOutputTest : public TestCase
{
  public:
    TmVoqSameOutputTest()
        : TestCase("TM: two inputs to same output are isolated VOQs")
    {
    }

    void DoRun() override
    {
        Ptr<P4TrafficManager> tm = MakeTm(3);

        // input 0 and input 2 both target output 1, same priority.
        Enq(tm, 100, 0, 1, 4, 10);
        Enq(tm, 100, 2, 1, 4, 20);

        // Distinct VOQs.
        NS_TEST_ASSERT_MSG_EQ(tm->VoqLength(0, 1, 4), 1, "VOQ[0][1][4] len 1");
        NS_TEST_ASSERT_MSG_EQ(tm->VoqLength(2, 1, 4), 1, "VOQ[2][1][4] len 1");
        NS_TEST_ASSERT_MSG_EQ(tm->InputBufferBytes(0), 100, "input 0 bytes");
        NS_TEST_ASSERT_MSG_EQ(tm->InputBufferBytes(2), 100, "input 2 bytes");

        // Output contention: only ONE of them can be granted this round
        // (one output receives at most one packet per round).
        auto grants = tm->RunFabricScheduler();
        NS_TEST_ASSERT_MSG_EQ(grants.size(), 1, "output 1 contended -> single grant");
        NS_TEST_ASSERT_MSG_EQ(grants[0].outPort, 1, "grant is for output 1");

        Simulator::Destroy();
    }
};

// ---------------------------------------------------------------------------
// 4. Fabric matching rules:
//    - one input sends at most one packet per round
//    - one output receives at most one packet per round
//    - a full permutation yields a maximal matching
// ---------------------------------------------------------------------------
class TmFabricMatchingTest : public TestCase
{
  public:
    TmFabricMatchingTest()
        : TestCase("TM: fabric one-in/one-out matching constraints")
    {
    }

    void DoRun() override
    {
        Ptr<P4TrafficManager> tm = MakeTm(4);

        // A perfect permutation demand: in i -> out (i+1)%4, all same priority.
        for (uint32_t i = 0; i < 4; ++i)
        {
            Enq(tm, 100, i, (i + 1) % 4, 3, i);
        }

        auto grants = tm->RunFabricScheduler();
        NS_TEST_ASSERT_MSG_EQ(grants.size(), 4, "permutation -> 4 grants (maximal)");

        std::vector<bool> inSeen(4, false), outSeen(4, false);
        for (const auto& g : grants)
        {
            NS_TEST_ASSERT_MSG_EQ(inSeen[g.inPort], false, "each input granted at most once");
            NS_TEST_ASSERT_MSG_EQ(outSeen[g.outPort], false, "each output granted at most once");
            inSeen[g.inPort] = true;
            outSeen[g.outPort] = true;
        }

        // Now a contention case: inputs 0,1,2 all want output 0.
        Ptr<P4TrafficManager> tm2 = MakeTm(4);
        Enq(tm2, 100, 0, 0, 3, 0);
        Enq(tm2, 100, 1, 0, 3, 1);
        Enq(tm2, 100, 2, 0, 3, 2);
        auto g2 = tm2->RunFabricScheduler();
        NS_TEST_ASSERT_MSG_EQ(g2.size(), 1, "3 inputs -> 1 output => single grant");
        NS_TEST_ASSERT_MSG_EQ(g2[0].outPort, 0, "grant for output 0");
        // Priority-first, then input order: input 0 wins the tie.
        NS_TEST_ASSERT_MSG_EQ(g2[0].inPort, 0, "input 0 wins tie (lowest index first)");

        Simulator::Destroy();
    }
};

// ---------------------------------------------------------------------------
// 5. Buffer overflow and drop-reason correctness
// ---------------------------------------------------------------------------
class TmBufferDropTest : public TestCase
{
  public:
    TmBufferDropTest()
        : TestCase("TM: finite buffers produce correct drop reasons")
    {
    }

    void DoRun() override
    {
        // ---- VOQ limit ----
        {
            Ptr<P4TrafficManager> tm = MakeTm(2);
            tm->SetAttribute("VoqLimit", UintegerValue(150));

            uint8_t lastReason = 255;
            tm->TraceConnectWithoutContext(
                "Drop",
                MakeCallback(&TmBufferDropTest::OnDrop, this));
            m_lastReason = &lastReason;

            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 1, 0), true, "first fits under VoqLimit");
            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 1, 0), false, "second exceeds VoqLimit -> drop");
            NS_TEST_ASSERT_MSG_EQ(lastReason,
                                  static_cast<uint8_t>(TmDropReason::VOQ_QUEUE_FULL),
                                  "drop reason == VOQ_QUEUE_FULL");
            NS_TEST_ASSERT_MSG_EQ(tm->VoqLength(0, 1, 0), 1, "only one packet accepted");
            const auto& s = tm->GetStats();
            NS_TEST_ASSERT_MSG_EQ(s.totalDropped, 1, "one drop counted");
            NS_TEST_ASSERT_MSG_EQ(
                s.dropsByReason[static_cast<size_t>(TmDropReason::VOQ_QUEUE_FULL)],
                1,
                "VOQ_QUEUE_FULL counter == 1");
            m_lastReason = nullptr;
        }

        // ---- Input buffer limit (independent of a single VOQ) ----
        {
            Ptr<P4TrafficManager> tm = MakeTm(3);
            tm->SetAttribute("InputBufferLimit", UintegerValue(150));
            uint8_t lastReason = 255;
            tm->TraceConnectWithoutContext(
                "Drop",
                MakeCallback(&TmBufferDropTest::OnDrop, this));
            m_lastReason = &lastReason;

            // Two different VOQs of the same input; second overflows input budget.
            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 1, 0), true, "input budget ok");
            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 2, 5), false, "input budget exceeded -> drop");
            NS_TEST_ASSERT_MSG_EQ(lastReason,
                                  static_cast<uint8_t>(TmDropReason::VOQ_INPUT_BUFFER_FULL),
                                  "drop reason == VOQ_INPUT_BUFFER_FULL");
            m_lastReason = nullptr;
        }

        // ---- Global buffer limit ----
        {
            Ptr<P4TrafficManager> tm = MakeTm(3);
            tm->SetAttribute("GlobalBufferLimit", UintegerValue(150));
            uint8_t lastReason = 255;
            tm->TraceConnectWithoutContext(
                "Drop",
                MakeCallback(&TmBufferDropTest::OnDrop, this));
            m_lastReason = &lastReason;

            // Different inputs, different VOQs; global budget is the binding limit.
            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 0, 1, 0), true, "global budget ok");
            NS_TEST_ASSERT_MSG_EQ(Enq(tm, 100, 1, 2, 0), false, "global budget exceeded -> drop");
            NS_TEST_ASSERT_MSG_EQ(lastReason,
                                  static_cast<uint8_t>(TmDropReason::VOQ_GLOBAL_BUFFER_FULL),
                                  "drop reason == VOQ_GLOBAL_BUFFER_FULL");
            m_lastReason = nullptr;
        }

        Simulator::Destroy();
    }

  private:
    void OnDrop(uint8_t reason, uint32_t, uint32_t, uint8_t, uint32_t)
    {
        if (m_lastReason)
        {
            *m_lastReason = reason;
        }
    }

    uint8_t* m_lastReason{nullptr};
};

// ---------------------------------------------------------------------------
// 6. Delay measurement correctness (VOQ waiting delay via simulated time)
// ---------------------------------------------------------------------------
class TmDelayMeasurementTest : public TestCase
{
  public:
    TmDelayMeasurementTest()
        : TestCase("TM: VOQ waiting delay is measured in simulated time")
    {
    }

    void DoRun() override
    {
        Ptr<P4TrafficManager> tm = MakeTm(2);
        tm->TraceConnectWithoutContext("VoqWaitingDelay",
                                       MakeCallback(&TmDelayMeasurementTest::OnDelay, this));

        // Enqueue at t=0, dequeue at t=500ns.
        Simulator::Schedule(Time(0), [tm]() { Enq(tm, 100, 0, 1, 3, 7); });
        Simulator::Schedule(NanoSeconds(500), [tm]() {
            TmItem item;
            tm->DequeueFromVoq(0, 1, 3, item);
        });

        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_lastDelay, NanoSeconds(500), "VOQ waiting delay == 500ns");
        const auto& s = tm->GetStats();
        NS_TEST_ASSERT_MSG_EQ(s.AvgVoqDelay(), NanoSeconds(500), "avg VOQ delay == 500ns");
        NS_TEST_ASSERT_MSG_EQ(s.maxQueueingDelay, NanoSeconds(500), "max queueing delay == 500ns");

        Simulator::Destroy();
    }

  private:
    void OnDelay(Time d)
    {
        m_lastDelay = d;
    }

    Time m_lastDelay;
};

// ---------------------------------------------------------------------------
// Suite
// ---------------------------------------------------------------------------
class P4TrafficManagerTestSuite : public TestSuite
{
  public:
    P4TrafficManagerTestSuite()
        : TestSuite("p4-traffic-manager", Type::UNIT)
    {
        AddTestCase(new TmBasicEnqueueDequeueTest, TestCase::QUICK);
        AddTestCase(new TmPrioritySchedulingTest, TestCase::QUICK);
        AddTestCase(new TmVoqSameOutputTest, TestCase::QUICK);
        AddTestCase(new TmFabricMatchingTest, TestCase::QUICK);
        AddTestCase(new TmBufferDropTest, TestCase::QUICK);
        AddTestCase(new TmDelayMeasurementTest, TestCase::QUICK);
    }
};

static P4TrafficManagerTestSuite g_p4TrafficManagerTestSuite;
