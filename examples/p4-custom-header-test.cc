#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/custom-header.h"
#include "ns3/log.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("P4CustomHeaderTest");

int
main ()
{
  LogComponentEnable ("P4CustomHeaderTest", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating a custom header instance...");

  CustomHeader customHeader;
  customHeader.SetLayer (HeaderLayer::LAYER_3);
  customHeader.SetOperator (HeaderLayerOperator::ADD_BEFORE);

  customHeader.AddField ("Field1", 8); // 8-bit field
  customHeader.AddField ("Field2", 16); // 16-bit field
  customHeader.AddField ("Field3", 32); // 32-bit field

  NS_LOG_INFO ("Setting field values...");
  customHeader.SetField ("Field1", 0xAB);
  customHeader.SetField ("Field2", 0x1234);
  customHeader.SetField ("Field3", 0x89ABCDEF);

  NS_LOG_INFO ("Original Custom Header:");
  customHeader.Print (std::cout);
  std::cout << std::endl;

  NS_LOG_INFO ("Serializing header...");
  Buffer buffer;
  buffer.AddAtEnd (customHeader.GetSerializedSize ());
  NS_LOG_INFO ("Required serialized size: " << customHeader.GetSerializedSize ());

  Buffer::Iterator start = buffer.Begin ();
  NS_ASSERT_MSG (start.GetSize () >= customHeader.GetSerializedSize (),
                 "Buffer size is too small for serialization!");
  customHeader.Serialize (start);

  NS_LOG_INFO ("Deserializing into a new CustomHeader instance...");
  CustomHeader newHeader;
  newHeader.AddField ("Field1", 8);
  newHeader.AddField ("Field2", 16);
  newHeader.AddField ("Field3", 32);

  Buffer::Iterator readStart = buffer.Begin ();
  newHeader.Deserialize (readStart);

  NS_LOG_INFO ("Deserialized Custom Header:");
  newHeader.Print (std::cout);
  std::cout << std::endl;

  NS_LOG_INFO ("Checking field values...");
  NS_ASSERT_MSG (newHeader.GetField ("Field1") == 0xAB, "Field1 value mismatch!");
  NS_ASSERT_MSG (newHeader.GetField ("Field2") == 0x1234, "Field2 value mismatch!");
  NS_ASSERT_MSG (newHeader.GetField ("Field3") == 0x89ABCDEF, "Field3 value mismatch!");

  NS_LOG_INFO ("CustomHeader test completed successfully.");
  return 0;
}
