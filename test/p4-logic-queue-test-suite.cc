/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/p4-rr-pri-queue-disc.h"
#include "ns3/priority-port-tag.h"
#include "ns3/p4-queue-item.h"

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

// class P4QueueDiscTestItem : public P4QueueItem
// {
// public:
//   P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint16_t port, uint8_t priority);
//   void AddHeader () override;
//   bool Mark () override;
// };

// P4QueueDiscTestItem::P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint16_t port,
//                                           uint8_t priority)
//     : P4QueueItem (p, addr, 0)
// {
//   PriorityPortTag priority_port_tag = PriorityPortTag (priority, port);
//   p->ReplacePacketTag (priority_port_tag);
// }

// void
// P4QueueDiscTestItem::AddHeader ()
// {
//   // No header to add in this test item.
// }

// bool
// P4QueueDiscTestItem::Mark ()
// {
//   return false; // No marking needed for this test.
// }

class P4QueueDiscSetGetTestCase : public TestCase
{
public:
  P4QueueDiscSetGetTestCase ();

  virtual ~P4QueueDiscSetGetTestCase () = default;

private:
  void DoRun () override;

  /**
   * \brief Test the Set and Get methods of the NSP4PriQueueDisc class
   */
  void TestSetGet ();

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

  TestSetGet ();
  TestEnqueueDequeue ();
}

void
P4QueueDiscSetGetTestCase::TestSetGet ()
{
  // Create and initialize the NSP4PriQueueDisc object
  Ptr<NSP4PriQueueDisc> queue = CreateObject<NSP4PriQueueDisc> ();

  // Set the number of ports and priorities using attributes
  const uint8_t nbPorts = 6;
  const uint8_t nbPriorities = 5;
  queue->SetAttribute ("NumPorts", UintegerValue (nbPorts));
  queue->SetAttribute ("NumPriorities", UintegerValue (nbPriorities));

  // Initialize the queue discipline
  queue->Initialize ();

  // Test init default values
  uint32_t defaultCapacity = 1000;
  NS_TEST_ASSERT_MSG_EQ (queue->GetQueueCapacity (0, 0), defaultCapacity,
                         "Queue Default Capacity init fault!");
  NS_TEST_ASSERT_MSG_EQ (queue->GetQueueCapacity (5, 4), defaultCapacity,
                         "Queue Default Capacity init fault!");
  uint32_t defaultRate = 1000;
  NS_TEST_ASSERT_MSG_EQ (queue->GetQueueRate (0, 0), defaultRate, "Queue Default Rate init fault!");
  NS_TEST_ASSERT_MSG_EQ (queue->GetQueueRate (5, 4), defaultRate, "Queue Default Rate init fault!");

  // Test GetQueueCapacity and SetQueueCapacity
  for (uint8_t port = 0; port < nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < nbPriorities; ++priority)
        {

          uint32_t testCapacity = defaultCapacity + port * 10 + priority;
          NS_LOG_DEBUG ("Testing port " << static_cast<int> (port) << ", priority "
                                        << static_cast<int> (priority) << ", capacity "
                                        << testCapacity);
          queue->SetQueueCapacity (port, priority, testCapacity);
          uint32_t retrievedCapacity = queue->GetQueueCapacity (port, priority);
          NS_TEST_ASSERT_MSG_EQ (retrievedCapacity, testCapacity, "Queue capacity mismatch!");
        }
    }

  // Test SetQueueRate and GetQueueRate
  const uint64_t defaultRatePps = 1000;
  for (uint8_t port = 0; port < nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < nbPriorities; ++priority)
        {
          uint64_t testRate = defaultRatePps + port * 100 + priority * 10;
          NS_LOG_DEBUG ("Testing port " << static_cast<int> (port) << ", priority "
                                        << static_cast<int> (priority) << ", rate " << testRate);
          queue->SetQueueRate (port, priority, testRate);
          uint64_t retrievedRate = queue->GetQueueRate (port, priority);
          NS_TEST_ASSERT_MSG_EQ (retrievedRate, testRate, "Queue rate mismatch!");
        }
    }
}

void
P4QueueDiscSetGetTestCase::TestEnqueueDequeue ()
{
  // Create and initialize the NSP4PriQueueDisc object
  Ptr<NSP4PriQueueDisc> queue = CreateObject<NSP4PriQueueDisc> ();
  const uint8_t nbPorts = 2;
  const uint8_t nbPriorities = 3;
  queue->SetAttribute ("NumPorts", UintegerValue (nbPorts));
  queue->SetAttribute ("NumPriorities", UintegerValue (nbPriorities));
  queue->Initialize ();

  // Create a packet and a test item
  uint32_t pktSize = 1000;
  Address dest;
  uint16_t protocol = 0;
  Ptr<Packet> p1, p2, p3, p4, p5, p6;

  p1 = Create<Packet> (pktSize);
  p2 = Create<Packet> (pktSize);
  p3 = Create<Packet> (pktSize);
  p4 = Create<Packet> (pktSize);
  p5 = Create<Packet> (pktSize);
  p6 = Create<Packet> (pktSize);

  PriorityPortTag pp_tag_0_0 = PriorityPortTag (0, 0);
  PriorityPortTag pp_tag_0_1 = PriorityPortTag (1, 0);
  PriorityPortTag pp_tag_0_2 = PriorityPortTag (2, 0);
  p1->ReplacePacketTag (pp_tag_0_0);
  p2->ReplacePacketTag (pp_tag_0_1);
  p3->ReplacePacketTag (pp_tag_0_2);

  // test queue length for port 0 with all virtual queues
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 0), 0,
                         "There should be no packets in queue for port 0, priority 0");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 1), 0,
                         "There should be no packets in queue for port 0, priority 1");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 2), 0,
                         "There should be no packets in queue for port 0, priority 2");

  queue->Enqueue (Create<P4QueueItem> (p1, dest, protocol)); // port 0, priority 0
  queue->Enqueue (Create<P4QueueItem> (p2, dest, protocol)); // port 0, priority 1
  queue->Enqueue (Create<P4QueueItem> (p3, dest, protocol)); // port 0, priority 2

  // test queue length for port 0 with all virtual queues
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 0), 1,
                         "There should be one packets in queue for port 0, priority 0");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 1), 1,
                         "There should be one packets in queue for port 0, priority 1");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueSize (0, 2), 1,
                         "There should be one packets in queue for port 0, priority 2");

  // test dequeue operation
  Ptr<QueueDiscItem> item = queue->Dequeue ();
  Ptr<Packet> de_null = item->GetPacket ();
  NS_TEST_ASSERT_MSG_EQ (de_null, nullptr, "No packets is dequeued!");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 3,
                         "There should be 3 packets left in total for port 0");

  Simulator::Stop (MicroSeconds (10));
  Simulator::Run ();

  // Perform Dequeue operation
  item = queue->Dequeue ();
  NS_LOG_DEBUG ("Dequeued packet from port 0 at time: " << Simulator::Now ().GetMilliSeconds ()
                                                        << " ms");

  // Before enqueue, we add Tags for the packets, that makes the packet different.
  item = queue->Dequeue ();
  Ptr<Packet> de_p1 = item->GetPacket ();
  de_p1->RemoveAllPacketTags ();
  NS_TEST_ASSERT_MSG_EQ (de_p1, p1, "Dequeued packet mismatch!");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 2,
                         "There should be 2 packets left in total for port 0");

  item = queue->Dequeue ();
  Ptr<Packet> de_p2 = item->GetPacket ();
  de_p2->RemoveAllPacketTags ();
  NS_TEST_ASSERT_MSG_EQ (de_p2, p2, "Dequeued packet mismatch!");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 1,
                         "There should be 1 packets left in total for port 0");

  item = queue->Dequeue ();
  Ptr<Packet> de_p3 = item->GetPacket ();
  de_p3->RemoveAllPacketTags ();
  NS_TEST_ASSERT_MSG_EQ (de_p3, p3, "Dequeued packet mismatch!");
  NS_TEST_EXPECT_MSG_EQ (queue->GetQueueTotalLengthPerPort (0), 0,
                         "There should be no packets left in total for port 0");
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
