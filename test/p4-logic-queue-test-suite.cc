/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/p4-rr-pri-queue-disc.h"
#include "ns3/priority-port-tag.h"
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
     * \param port the port
     */
  P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint8_t priority, uint16_t port);
  void AddHeader () override;
  bool Mark () override;
};

P4QueueDiscTestItem::P4QueueDiscTestItem (Ptr<Packet> p, const Address &addr, uint8_t priority,
                                          uint16_t port)
    : QueueDiscItem (p, addr, 0)
{
  // 将端口信息作为一个独立的Tag加入
  PriorityPortTag priority_port_tag;
  priority_port_tag.SetPriority (priority);
  priority_port_tag.SetPort (port);

  p->ReplacePacketTag (priority_port_tag);
}

void
P4QueueDiscTestItem::AddHeader ()
{
  // No header to add in this test item.
}

bool
P4QueueDiscTestItem::Mark ()
{
  return false; // No marking needed for this test.
}

class P4QueueDiscSetGetTestCase : public TestCase
{
public:
  P4QueueDiscSetGetTestCase ();

  virtual ~P4QueueDiscSetGetTestCase () = default;

private:
  void DoRun () override;

  // Helper functions for assertions
  void TestSetGet (Ptr<NSP4PriQueueDisc> qdisc);
};

P4QueueDiscSetGetTestCase::P4QueueDiscSetGetTestCase ()
    : TestCase ("Sanity check on the multi-port priority queue disc implementation")
{
}

void
P4QueueDiscSetGetTestCase::DoRun ()
{
  // Create and initialize the NSP4PriQueueDisc object
  Ptr<NSP4PriQueueDisc> qdisc = CreateObject<NSP4PriQueueDisc> ();

  TestSetGet (qdisc);
}

void
P4QueueDiscSetGetTestCase::TestSetGet (Ptr<NSP4PriQueueDisc> qdisc)
{
  // Set the number of ports and priorities using attributes
  const uint8_t nbPorts = 6;
  const uint8_t nbPriorities = 5;
  qdisc->SetAttribute ("NumPorts", UintegerValue (nbPorts));
  qdisc->SetAttribute ("NumPriorities", UintegerValue (nbPriorities));

  // Initialize the queue discipline
  qdisc->Initialize ();

  // Test GetQueueCapacity and SetQueueCapacity
  const uint32_t defaultCapacity = 100;
  for (uint8_t port = 0; port < nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < nbPriorities; ++priority)
        {
          std::cout << "Testing port " << port << ", priority " << priority;
          uint32_t testCapacity = defaultCapacity + port * 10 + priority;
          qdisc->SetQueueCapacity (port, priority, testCapacity);
          uint32_t retrievedCapacity = qdisc->GetQueueCapacity (port, priority);
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
          qdisc->SetQueueRate (port, priority, testRate);
          uint64_t retrievedRate = qdisc->GetQueueRate (port, priority);
          NS_TEST_ASSERT_MSG_EQ (retrievedRate, testRate, "Queue rate mismatch!");
        }
    }

  // Test GetVirtualQueueLengthPerPort
  for (uint8_t port = 0; port < nbPorts; ++port)
    {
      for (uint8_t priority = 0; priority < nbPriorities; ++priority)
        {
          uint32_t expectedLength = qdisc->GetQueueSize (port, priority);
          uint32_t retrievedLength = qdisc->GetVirtualQueueLengthPerPort (port, priority);
          NS_TEST_ASSERT_MSG_EQ (retrievedLength, expectedLength, "Virtual queue length mismatch!");
        }
    }
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
