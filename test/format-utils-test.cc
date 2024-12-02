/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "format-utils.h"

#include "ns3/log.h"
#include "ns3/test.h"

#include <string>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FormatUtilsTest");

/**
 * @brief TestCase for testing format-utils.h functions
 */
class FormatUtilsTestCase : public TestCase
{
  public:
    FormatUtilsTestCase();
    virtual ~FormatUtilsTestCase();

  private:
    virtual void DoRun() override;

    // Helper functions for assertions
    void TestIntToStr();
    void TestStrToInt();
    void TestStrToDouble();
    void TestHexCharToInt();
    void TestHexStrToBytes();
    void TestIpStrToBytes();
    void TestUint32IpToHex();
    void TestIntToBytes();
};

// Constructor
FormatUtilsTestCase::FormatUtilsTestCase()
    : TestCase("FormatUtils TestCase") // Test case description
{
}

// Destructor
FormatUtilsTestCase::~FormatUtilsTestCase()
{
}

// Test runner
void
FormatUtilsTestCase::DoRun()
{
    TestIntToStr();
    TestStrToInt();
    TestStrToDouble();
    TestHexCharToInt();
    TestHexStrToBytes();
    TestIpStrToBytes();
    TestUint32IpToHex();
    TestIntToBytes();
}

/**
 * @brief Test IntToStr function
 */
void
FormatUtilsTestCase::TestIntToStr()
{
    char* result = IntToStr(12345);
    NS_TEST_ASSERT_MSG_EQ(std::string(result), "12345", "IntToStr failed for positive numbers");
    delete[] result;

    result = IntToStr(-6789);
    NS_TEST_ASSERT_MSG_EQ(std::string(result), "-6789", "IntToStr failed for negative numbers");
    delete[] result;

    result = IntToStr(0);
    NS_TEST_ASSERT_MSG_EQ(std::string(result), "0", "IntToStr failed for zero");
    delete[] result;
}

/**
 * @brief Test StrToInt function
 */
void
FormatUtilsTestCase::TestStrToInt()
{
    NS_TEST_ASSERT_MSG_EQ(StrToInt("12345"), 12345u, "StrToInt failed for decimal numbers");
    NS_TEST_ASSERT_MSG_EQ(StrToInt("0x1a2b"), 6699u, "StrToInt failed for hexadecimal numbers");
    NS_TEST_ASSERT_MSG_EQ(StrToInt("0b1101"), 13u, "StrToInt failed for binary numbers");

    NS_TEST_ASSERT_MSG_EQ(StrToInt("0"), 0u, "StrToInt failed for zero");
}

/**
 * @brief Test StrToDouble function
 */
void
FormatUtilsTestCase::TestStrToDouble()
{
    NS_TEST_ASSERT_MSG_EQ_TOL(StrToDouble("12.34"),
                              12.34,
                              1e-6,
                              "StrToDouble failed for decimal values");
    NS_TEST_ASSERT_MSG_EQ_TOL(StrToDouble("0.00123"),
                              0.00123,
                              1e-6,
                              "StrToDouble failed for small decimal values");
}

/**
 * @brief Test HexCharToInt function
 */
void
FormatUtilsTestCase::TestHexCharToInt()
{
    NS_TEST_ASSERT_MSG_EQ(HexCharToInt('a'), 10, "HexCharToInt failed for lowercase hex");
    NS_TEST_ASSERT_MSG_EQ(HexCharToInt('F'), 15, "HexCharToInt failed for uppercase hex");
    NS_TEST_ASSERT_MSG_EQ(HexCharToInt('0'), 0, "HexCharToInt failed for digit 0");
}

/**
 * @brief Test HexStrToBytes function
 */
void
FormatUtilsTestCase::TestHexStrToBytes()
{
    NS_TEST_ASSERT_MSG_EQ(HexStrToBytes("0x0a010001"),
                          "\x0a\x01\x00\x01",
                          "HexStrToBytes failed for valid input");
    NS_TEST_ASSERT_MSG_EQ(HexStrToBytes("ff00"),
                          "\xff\x00",
                          "HexStrToBytes failed for short input");
}

/**
 * @brief Test IpStrToBytes function
 */
void
FormatUtilsTestCase::TestIpStrToBytes()
{
    NS_TEST_ASSERT_MSG_EQ(IpStrToBytes("10.1.0.1"),
                          "\x0a\x01\x00\x01",
                          "IpStrToBytes failed for valid IP address");
}

/**
 * @brief Test Uint32IpToHex function
 */
void
FormatUtilsTestCase::TestUint32IpToHex()
{
    NS_TEST_ASSERT_MSG_EQ(Uint32IpToHex(167772161),
                          "0x0a010001",
                          "Uint32IpToHex failed for valid input");
    NS_TEST_ASSERT_MSG_EQ(Uint32IpToHex(0), "0x00000000", "Uint32IpToHex failed for zero input");
}

/**
 * @brief Test IntToBytes function
 */
void
FormatUtilsTestCase::TestIntToBytes()
{
    NS_TEST_ASSERT_MSG_EQ(IntToBytes("123", 16), "\x00\x7b", "IntToBytes failed for 16-bit width");
    NS_TEST_ASSERT_MSG_EQ(IntToBytes("255", 8), "\xff", "IntToBytes failed for 8-bit width");
}

/**
 * @brief TestSuite for format-utils.h
 */
class FormatUtilsTestSuite : public TestSuite
{
  public:
    FormatUtilsTestSuite();
};

FormatUtilsTestSuite::FormatUtilsTestSuite()
    : TestSuite("format-utils", UNIT) // Test suite name and type
{
    AddTestCase(new FormatUtilsTestCase, TestCase::QUICK);
}

// Register the test suite with NS-3
static FormatUtilsTestSuite formatUtilsTestSuite;

} // namespace ns3
