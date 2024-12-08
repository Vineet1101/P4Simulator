/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/p4-rr-pri-queue-disc.h"
#include "ns3/priority-port-tag.h"

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/assert.h"

#include "ns3/packet-filter.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/address.h"
#include "ns3/tag.h"

#include <array>
#include <queue>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4QueueDiscTest");

class P4QueueDiscTestItem : public QueueDiscItem
{
public:
  /**
   * Constructor
   *
   * \param p the packet
   * \param addr the address
   * \param priority the packet priority
   */
  P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint16_t port, uint8_t priority);
  virtual ~P4QueueDiscTestItem ();
  virtual void AddHeader (void);
  virtual bool Mark (void);
};

P4QueueDiscTestItem::P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint16_t port,
                                          uint8_t priority)
    : QueueDiscItem (p, addr, 0)
{
  PriorityPortTag priority_port_tag = PriorityPortTag (priority, port);
  p->ReplacePacketTag (priority_port_tag);
}

P4QueueDiscTestItem::~P4QueueDiscTestItem ()
{
}

void
P4QueueDiscTestItem::AddHeader ()
{
}

bool
P4QueueDiscTestItem::Mark ()
{
  return false;
}

class P4QueueDiscSetGetTestCase : public TestCase
{
public:
  P4QueueDiscSetGetTestCase ();

  virtual ~P4QueueDiscSetGetTestCase () = default;

private:
  void DoRun () override;

  /**
   * \brief Test the Enqueue methods of the NSP4PriQueueDisc class with priority tags
   *
   */
  void TestEnqueueDequeue ();
};

P4QueueDiscSetGetTestCase::P4QueueDiscSetGetTestCase ()
    : TestCase ("Sanity check on the multi-port priority queue disc implementation")
{
}

void
P4QueueDiscSetGetTestCase::DoRun ()
{

  TestEnqueueDequeue ();
}

void
P4QueueDiscSetGetTestCase::TestEnqueueDequeue ()
{
  // Create and initialize the NSP4PriQueueDisc object
  Ptr<NSP4PriQueueDisc> queue = CreateObject<NSP4PriQueueDisc> ();
  const uint8_t nbPorts = 2;
  // const uint8_t nbPriorities = 3;
  queue->SetAttribute ("NumPorts", UintegerValue (nbPorts));
  // queue->SetAttribute ("NumPriorities", UintegerValue (nbPriorities));
  queue->Initialize ();

  // Create a packet and a test item
  uint32_t pktSize = 1000;
  Address dest;
  Ptr<Packet> p1, p2, p3, p4, p5, p6;

  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (pktSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);

  // test queue length for port 0 with all virtual queues
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 0), 0,
                         "There should be no packets in queue for port 0, priority 0");
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 1), 0,
                         "There should be no packets in queue for port 0, priority 1");
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 2), 0,
                         "There should be no packets in queue for port 0, priority 2");

  queue->Enqueue (Create<P4QueueDiscTestItem> (p1, dest, 0, 0)); // port 0, priority 0
  queue->Enqueue (Create<P4QueueDiscTestItem> (p2, dest, 0, 1)); // port 0, priority 1
  queue->Enqueue (Create<P4QueueDiscTestItem> (p3, dest, 0, 2)); // port 0, priority 2

  // test queue length for port 0 with all virtual queues
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 0), 1,
                         "There should be one packets in queue for port 0, priority 0");
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 1), 1,
                         "There should be one packets in queue for port 0, priority 1");
  NS_TEST_EXPECT_MSG_EQ (queue->GetVirtualQueueLengthPerPort (0, 2), 1,
                         "There should be one packets in queue for port 0, priority 2");

  // // test dequeue operation
  // Ptr<QueueDiscItem> item = queue->Dequeue ();
  // Ptr<Packet> de_null = item->GetPacket ();
  // NS_TEST_ASSERT_MSG_EQ (de_null, nullptr, "No packets is dequeued!");
  // NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 3,
  //                        "There should be 3 packets left in total for port 0");

  // Simulator::Stop (MicroSeconds (10));
  // Simulator::Run ();

  // // Perform Dequeue operation
  // item = queue->Dequeue ();
  // NS_LOG_DEBUG ("Dequeued packet from port 0 at time: " << Simulator::Now ().GetMilliSeconds ()
  //                                                       << " ms");

  // // Before enqueue, we add Tags for the packets, that makes the packet different.
  // item = queue->Dequeue ();
  // Ptr<Packet> de_p1 = item->GetPacket ();
  // de_p1->RemoveAllPacketTags ();
  // NS_TEST_ASSERT_MSG_EQ (de_p1, p1, "Dequeued packet mismatch!");
  // NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 2,
  //                        "There should be 2 packets left in total for port 0");

  // item = queue->Dequeue ();
  // Ptr<Packet> de_p2 = item->GetPacket ();
  // de_p2->RemoveAllPacketTags ();
  // NS_TEST_ASSERT_MSG_EQ (de_p2, p2, "Dequeued packet mismatch!");
  // NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 1,
  //                        "There should be 1 packets left in total for port 0");

  // item = queue->Dequeue ();
  // Ptr<Packet> de_p3 = item->GetPacket ();
  // de_p3->RemoveAllPacketTags ();
  // NS_TEST_ASSERT_MSG_EQ (de_p3, p3, "Dequeued packet mismatch!");
  // NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 0,
  //                        "There should be no packets left in total for port 0");
}

/**
 * \ingroup traffic-control-test
 *
 * \brief MultiPort Queue Disc Test Suite
 */
class P4QueueDiscTestSuite : public TestSuite
{
public:
  P4QueueDiscTestSuite () : TestSuite ("p4-queue-disc", Type::UNIT)
  {
    AddTestCase (new P4QueueDiscSetGetTestCase (), TestCase::QUICK);
  }
};

// Register the test suite with NS-3
static P4QueueDiscTestSuite p4QueueTestSuite;

} // namespace ns3
