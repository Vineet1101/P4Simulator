#include "p4-logic-pri-rl-queue-disc.h"
#include "priority-port-tag.h"

#include "ns3/log.h"
#include "ns3/packet-filter.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/address.h"
#include "ns3/tag.h"

#include <array>
#include <queue>

using namespace ns3;

#define P4SWITCH_VIRTUAL_QUEUE 8
#define P4SWITCH_CAPACITY 1000
#define P4SWTICH_PROCESS_RATE 1000

/**
 * \ingroup traffic-control-test
 *
 * \brief MultiPort Priority Queue Disc Test Item
 */
class NSQueueingLogicPriRLQueueDiscTestItem : public QueueDiscItem
{
public:
    /**
     * Constructor
     * 
     * \param p the packet
     * \param addr the address
     * \param priority the packet priority
     * \param port the port
     */
    NSQueueingLogicPriRLQueueDiscTestItem(Ptr<Packet> p, const Address& addr, uint8_t priority, uint16_t port);
    void AddHeader() override;
    bool Mark() override;
};

NSQueueingLogicPriRLQueueDiscTestItem::NSQueueingLogicPriRLQueueDiscTestItem(Ptr<Packet> p, const Address& addr, uint8_t priority, uint16_t port)
    : QueueDiscItem(p, addr, 0)
{
    // 将端口信息作为一个独立的Tag加入
    PriorityPortTag priority_port_tag;
    priority_port_tag.SetPriority(priority);
    priority_port_tag.SetPort(port);

    p->ReplacePacketTag(priority_port_tag);
}

void
NSQueueingLogicPriRLQueueDiscTestItem::AddHeader()
{
    // No header to add in this test item.
}

bool
NSQueueingLogicPriRLQueueDiscTestItem::Mark()
{
    return false; // No marking needed for this test.
}

/**
 * \ingroup traffic-control-test
 *
 * \brief MultiPort Priority Queue Disc Test Case
 */
class NSQueueingLogicPriRLQueueDiscTestCase : public TestCase
{
public:
  NSQueueingLogicPriRLQueueDiscTestCase();
  void DoRun() override;
};

NSQueueingLogicPriRLQueueDiscTestCase::NSQueueingLogicPriRLQueueDiscTestCase()
    : TestCase("Sanity check on the multi-port priority queue disc implementation")
{
}

void
NSQueueingLogicPriRLQueueDiscTestCase::DoRun()
{
    Ptr<NSQueueingLogicPriRLQueueDisc> qdisc = CreateObject<NSQueueingLogicPriRLQueueDisc>();
    qdisc->Initialize();

    
    Address dest;
    std::array<std::queue<uint32_t>, P4SWITCH_VIRTUAL_QUEUE> uids;

    // 测试 1: 设置优先级和端口, 测试是否正确设定
    for (size_t priority = 0; priority < P4SWITCH_VIRTUAL_QUEUE; ++priority)
    {
        qdisc->SetQueueCapacity(priority, P4SWITCH_CAPACITY);
        qdisc->SetQueueRate(priority, P4SWTICH_PROCESS_RATE);
        NS_TEST_ASSERT_MSG_EQ(qdisc->GetQueueCapacity(priority), P4SWITCH_CAPACITY,
                              "Queue capacity mismatch for priority " << priority);
        NS_TEST_ASSERT_MSG_EQ(qdisc->GetQueueRate(priority), P4SWTICH_PROCESS_RATE,
                                "Queue rate mismatch for priority " << priority);
    }

    // 测试 2: 根据优先级和端口进行分类
    for (uint16_t priority = 0; priority < P4SWITCH_VIRTUAL_QUEUE; ++priority)
    {
        for (uint16_t port = 0; port < P4SWITCH_VIRTUAL_QUEUE; ++port)
        {
            Ptr<Packet> packet = Create<Packet>(100);
            PriorityPortTag tag(priority, port);
            packet->AddPacketTag(tag);

            Ptr<QueueDiscItem> item = Create<QueueDiscItem>(packet, dest, 0); // 包装为 QueueDiscItem

            bool result = qdisc->Enqueue(item);
            NS_TEST_ASSERT_MSG_EQ(result, true, "Enqueue failed for priority " << priority << " and port " << port);

            uids[priority].push(packet->GetUid());
            NS_TEST_ASSERT_MSG_EQ(qdisc->GetQueueSize(priority), uids[priority].size(),
                                  "Queue size mismatch for priority " << priority);
        }
    }

    // 测试 3: 验证出队行为
    for (uint16_t priority = 0; priority < P4SWITCH_VIRTUAL_QUEUE; ++priority)
    {
        while (!uids[priority].empty())
        {
            Ptr<QueueDiscItem> dequeuedItem = qdisc->Dequeue();
            NS_TEST_ASSERT_MSG_NE(dequeuedItem, nullptr, "Dequeue returned null for priority " << priority);

            uint64_t expectedUid = uids[priority].front();
            uids[priority].pop();

            NS_TEST_ASSERT_MSG_EQ(dequeuedItem->GetPacket()->GetUid(), expectedUid,
                                  "Dequeued packet UID mismatch for priority " << priority);
        }

        NS_TEST_ASSERT_MSG_EQ(qdisc->GetQueueSize(priority), 0,
                              "Priority queue " << priority << " should be empty after dequeuing");
    }

    Simulator::Destroy();
}

/**
 * \ingroup traffic-control-test
 *
 * \brief MultiPort Queue Disc Test Suite
 */
static class NSQueueingLogicPriRLQueueDiscTestSuite : public TestSuite
{
  public:
    NSQueueingLogicPriRLQueueDiscTestSuite()
        : TestSuite("multi-port-prio-queue-disc", Type::UNIT)
    {
        AddTestCase(new NSQueueingLogicPriRLQueueDiscTestCase(), TestCase::QUICK);
    }
} g_multiPortQueueTestSuite; ///< the test suite
