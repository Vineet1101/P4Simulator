// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// // #include "ns3/p4-rr-pri-queue-disc.h"
// // #include "ns3/priority-port-tag.h"

// #include "ns3/p4-queue.h"
// #include "ns3/p4-queue-item.h"

// #include "ns3/test.h"
// #include "ns3/log.h"
// #include "ns3/assert.h"
// #include "ns3/rng-seed-manager.h"
// #include "ns3/random-variable-stream.h"

// #include <array>
// #include <queue>

// namespace ns3 {

// class P4QueueDiscSetGetTestCase : public TestCase
// {
// public:
//   P4QueueDiscSetGetTestCase ();

//   virtual ~P4QueueDiscSetGetTestCase () = default;

// private:
//   void DoRun () override;

//   void TestTwoTierP4QueueDisc ();

//   /**
//    * \brief Test the Enqueue methods of the NSP4PriQueueDisc class with priority tags
//    *
//    */
//   void TestP4QueueBufferEnqueueDequeue ();
// };

// P4QueueDiscSetGetTestCase::P4QueueDiscSetGetTestCase ()
//     : TestCase ("Sanity check on the multi-port priority queue disc implementation")
// {
// }

// void
// P4QueueDiscSetGetTestCase::DoRun ()
// {

//   TestTwoTierP4QueueDisc ();
//   TestP4QueueBufferEnqueueDequeue ();
// }

// void
// P4QueueDiscSetGetTestCase::TestTwoTierP4QueueDisc ()
// {
//   // Step 1: Create a TwoTierP4Queue instance
//   Ptr<TwoTierP4Queue> queue = CreateObject<TwoTierP4Queue> ();
//   queue->Initialize ();

//   // Ensure the queue is empty initially
//   NS_TEST_ASSERT_MSG_EQ (queue->IsEmpty (), true, "Queue should be empty after initialization.");

//   // Step 2: Create mock P4QueueItems and enqueue them
//   Ptr<P4QueueItem> highPriorityItem_1 =
//       Create<P4QueueItem> (Create<Packet> (100), PacketType::RECIRCULATE);
//   Ptr<P4QueueItem> highPriorityItem_2 =
//       Create<P4QueueItem> (Create<Packet> (150), PacketType::RESUBMIT);
//   Ptr<P4QueueItem> lowPriorityItem = Create<P4QueueItem> (Create<Packet> (200), PacketType::NORMAL);

//   // Enqueue items into the appropriate queues
//   NS_TEST_ASSERT_MSG_EQ (queue->Enqueue (lowPriorityItem), true,
//                          "Failed to enqueue low-priority item.");
//   NS_TEST_ASSERT_MSG_EQ (queue->Enqueue (highPriorityItem_1), true,
//                          "Failed to enqueue high-priority item.");
//   NS_TEST_ASSERT_MSG_EQ (queue->Enqueue (highPriorityItem_2), true,
//                          "Failed to enqueue high-priority item.");

//   // Ensure the queue is not empty
//   NS_TEST_ASSERT_MSG_EQ (queue->IsEmpty (), false,
//                          "Queue should not be empty after enqueuing items.");

//   // Step 3: Dequeue items and check priorities
//   Ptr<P4QueueItem> dequeuedItem;

//   // First dequeue should return the high-priority item 1
//   dequeuedItem = queue->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem, highPriorityItem_1,
//                          "Dequeued item should be the high-priority item 1.");

//   // Second dequeue should return the high-priority item 2
//   dequeuedItem = queue->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem, highPriorityItem_2,
//                          "Dequeued item should be the high-priority item 2.");

//   // Third dequeue should return the low-priority item
//   dequeuedItem = queue->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem, lowPriorityItem,
//                          "Dequeued item should be the low-priority item.");

//   // Ensure the queue is empty again
//   NS_TEST_ASSERT_MSG_EQ (queue->IsEmpty (), true,
//                          "Queue should be empty after dequeuing all items.");
// }

// void
// P4QueueDiscSetGetTestCase::TestP4QueueBufferEnqueueDequeue ()
// {
//   uint32_t random_seed = 1;
//   // Step 1: Create a P4Queuebuffer instance with 3 ports and 2 priority queues per port
//   uint32_t nPorts = 3;
//   uint32_t nPriorities = 2;
//   Ptr<P4Queuebuffer> queueBuffer = CreateObject<P4Queuebuffer> (nPorts, nPriorities);
//   // Step 2: Set a fixed random seed
//   queueBuffer->SetRandomSeed (random_seed);

//   /**
//    * Test the random number generator by generating 10 random values
//    * With random_seed = 1, the first ten random values should be 2, 2, 1, 2, 1, 0, 0, 0, 0, 0
//    * That means, the dequeue order is 2, 2, 1, (2), 1, 0, 0, and the high priority packets should
//    * first be dequeued, then the low priority packets. In the order there is a "(2)"",
//    * Because the thrid 2 is empty, the next random value is 1.
//    */

//   // // Print the first 10 random values
//   // Ptr<UniformRandomVariable> temp_rng = CreateObject<UniformRandomVariable> ();
//   // temp_rng->SetStream (random_seed);
//   // for (int i = 0; i < 10; ++i)
//   //   {
//   //     int randomValue = temp_rng->GetInteger (0, nPorts - 1);
//   //     std::cout << "Random value " << i + 1 << ": " << randomValue << std::endl;
//   //   }

//   // Step 3: Enqueue packets into the buffer
//   Ptr<P4QueueItem> item0_0 =
//       Create<P4QueueItem> (Create<Packet> (100), PacketType::NORMAL); // For port 0, priority 0
//   Ptr<P4QueueItem> item0_1 =
//       Create<P4QueueItem> (Create<Packet> (150), PacketType::NORMAL); // For port 0, priority 1

//   item0_0->SetMetadataEgressPort (0);
//   item0_0->SetMetadataPriority (0);
//   item0_1->SetMetadataEgressPort (0);
//   item0_1->SetMetadataPriority (1);

//   Ptr<P4QueueItem> item1_0 =
//       Create<P4QueueItem> (Create<Packet> (200), PacketType::NORMAL); // For port 1, priority 0
//   Ptr<P4QueueItem> item1_1 =
//       Create<P4QueueItem> (Create<Packet> (250), PacketType::NORMAL); // For port 1, priority 1

//   item1_0->SetMetadataEgressPort (1);
//   item1_0->SetMetadataPriority (0);
//   item1_1->SetMetadataEgressPort (1);
//   item1_1->SetMetadataPriority (1);

//   Ptr<P4QueueItem> item2_0 = Create<P4QueueItem> (Create<Packet> (300),
//                                                   PacketType::NORMAL); // For port 2, priority 0
//   Ptr<P4QueueItem> item2_1 =
//       Create<P4QueueItem> (Create<Packet> (350), PacketType::NORMAL); // For port 2, priority 1

//   item2_0->SetMetadataEgressPort (2);
//   item2_0->SetMetadataPriority (0);
//   item2_1->SetMetadataEgressPort (2);
//   item2_1->SetMetadataPriority (1);

//   // first enqueue low priority packets
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item0_0), true, "Failed to enqueue item0_0.");
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item0_1), true, "Failed to enqueue item0_1.");
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item1_0), true, "Failed to enqueue item1_0.");
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item1_1), true, "Failed to enqueue item1_1.");
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item2_0), true, "Failed to enqueue item2_0.");
//   NS_TEST_ASSERT_MSG_EQ (queueBuffer->Enqueue (item2_1), true, "Failed to enqueue item2_1.");

//   // Step 4: Dequeue packets and verify correct random port selection and priority behavior
//   Ptr<P4QueueItem> dequeuedItem;

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 350,
//                          "Dequeued wrong item for the first dequeue.");

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 300,
//                          "Dequeued wrong item for the second dequeue.");

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 250,
//                          "Dequeued wrong item for the second dequeue.");

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 200,
//                          "Dequeued wrong item for the second dequeue.");

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 150,
//                          "Dequeued wrong item for the second dequeue.");

//   dequeuedItem = queueBuffer->Dequeue ();
//   NS_TEST_ASSERT_MSG_NE (dequeuedItem, nullptr, "Dequeued item should not be null.");
//   NS_TEST_ASSERT_MSG_EQ (dequeuedItem->GetPacket ()->GetSize (), 100,
//                          "Dequeued wrong item for the second dequeue.");

//   // Ensure the buffer is empty after dequeuing all items
//   for (uint32_t port = 0; port < nPorts; ++port)
//     {
//       NS_TEST_ASSERT_MSG_EQ (queueBuffer->IsEmpty (port), true,
//                              "Port " << port << " should be empty after dequeuing all items.");
//     }
// }

// /**
//  * \ingroup traffic-control-test
//  *
//  * \brief MultiPort Queue Disc Test Suite
//  */
// class P4QueueDiscTestSuite : public TestSuite
// {
// public:
//   P4QueueDiscTestSuite () : TestSuite ("p4-queue-disc", Type::UNIT)
//   {
//     AddTestCase (new P4QueueDiscSetGetTestCase (), TestCase::QUICK);
//   }
// };

// // Register the test suite with NS-3
// static P4QueueDiscTestSuite p4QueueTestSuite;

// } // namespace ns3
