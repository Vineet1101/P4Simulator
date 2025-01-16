#include "custom-header.h"
#include "ns3/global.h"
#include "ns3/log.h"
#include "ns3/string.h" // For StringValue
#include "ns3/attribute.h" // For MakeStringAccessor and MakeStringChecker

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomHeader");

TypeId
CustomHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomHeader")
                          .SetParent<Header> ()
                          .SetGroupName ("Custom")
                          .AddConstructor<CustomHeader> ();
  return tid;
}

CustomHeader::CustomHeader () : m_protocol_number (0)
{
  InitFields (); // init based on GlobalVar::g_templateHeaderFields
}

CustomHeader::CustomHeader (const CustomHeader &other)
    : Header (),
      m_layer (other.m_layer),
      m_op (other.m_op),
      m_protocol_number (other.m_protocol_number),
      m_fields (other.m_fields) // Use std::vector's copy constructor
{
  NS_LOG_DEBUG ("Copy constructor called");
}

CustomHeader::~CustomHeader ()
{
}

void
CustomHeader::InitFields ()
{
  m_fields.clear ();
  for (const auto &field : P4GlobalVar::g_templateHeaderFields)
    {
      m_fields.push_back ({std::string (field.first), field.second, 0});
    }
}

CustomHeader &
CustomHeader::operator= (const CustomHeader &other)
{
  if (this == &other)
    {
      return *this; // Self-assignment protection
    }

  // Header::operator= (other);

  // Copy other members
  m_layer = other.m_layer;
  m_op = other.m_op;
  m_protocol_number = other.m_protocol_number;
  m_fields = other.m_fields; // Use std::vector's copy constructor

  NS_LOG_DEBUG ("Assignment operator called");
  return *this;
}

void
CustomHeader::AddField (const std::string &name, uint32_t bitWidth)
{
  if (bitWidth > 64)
    {
      throw std::invalid_argument ("Bit width cannot exceed 64 bits.");
    }
  Field field{name, bitWidth, 0};
  m_fields.push_back (field);
}

void
CustomHeader::SetField (const std::string &name, uint64_t value)
{
  for (auto &field : m_fields)
    {
      if (field.name == name)
        {
          if (value >= (1ULL << field.bitWidth))
            {
              throw std::out_of_range ("Value exceeds the maximum allowed by field width.");
            }
          field.value = value;
          return;
        }
    }
  throw std::invalid_argument ("Field not found: " + name);
}

uint64_t
CustomHeader::GetField (const std::string &name) const
{
  for (const auto &field : m_fields)
    {
      if (field.name == name)
        {
          return field.value;
        }
    }
  throw std::invalid_argument ("Field not found: " + name);
}

void
CustomHeader::SetProtocolFieldNumber (uint64_t id)
{
  if (m_fields.empty ()) // 判断是否已经赋值
    {
      NS_LOG_WARN ("m_fields is empty! Set protocol number.");
    }

  else if (id >= m_fields.size ())
    {
      NS_LOG_WARN ("Invalid protocol number assignment: id = " << id << ", but m_fields size = "
                                                               << m_fields.size ());
      return;
    }

  m_protocol_number = id;
}

uint64_t
CustomHeader::GetProtocolNumber ()
{
  NS_LOG_INFO ("Protocol number: " << m_protocol_number);

  if (m_protocol_number >= m_fields.size ())
    {
      NS_LOG_ERROR ("Index out of bounds: m_protocol_number = "
                    << m_protocol_number << ", but m_fields size = " << m_fields.size ());
      return 0;
    }

  return m_fields[m_protocol_number].value;
}

void
CustomHeader::SetLayer (HeaderLayer layer)
{
  m_layer = layer;
}

HeaderLayer
CustomHeader::GetLayer () const
{
  return m_layer;
}

void
CustomHeader::SetOperator (HeaderLayerOperator op)
{
  m_op = op;
}

HeaderLayerOperator
CustomHeader::GetOperator () const
{
  return m_op;
}

TypeId
CustomHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CustomHeader::Serialize (Buffer::Iterator start) const
{
  uint32_t currentBit = 0;
  uint8_t currentByte = 0;

  for (const auto &field : m_fields)
    {
      uint32_t bitsToWrite = field.bitWidth;
      uint64_t value = field.value;

      while (bitsToWrite > 0)
        {
          uint32_t freeBits = 8 - currentBit;
          uint32_t bitsToWriteNow = std::min (freeBits, bitsToWrite);

          currentByte |= ((value >> (bitsToWrite - bitsToWriteNow)) & ((1 << bitsToWriteNow) - 1))
                         << (freeBits - bitsToWriteNow);
          bitsToWrite -= bitsToWriteNow;
          currentBit += bitsToWriteNow;

          if (currentBit == 8)
            {
              start.WriteU8 (currentByte);
              currentByte = 0;
              currentBit = 0;
            }
        }
    }

  if (currentBit > 0)
    {
      start.WriteU8 (currentByte);
    }
}

uint32_t
CustomHeader::Deserialize (Buffer::Iterator start)
{
  uint32_t totalBits = 0;
  for (const auto &field : m_fields)
    {
      totalBits += field.bitWidth;
    }

  uint32_t bytesToRead = (totalBits + 7) / 8;

  std::vector<uint8_t> buffer (bytesToRead);

  NS_LOG_DEBUG ("Deserializing " << bytesToRead << " bytes...");
  for (uint32_t i = 0; i < bytesToRead; ++i)
    {
      buffer[i] = start.ReadU8 ();
    }

  uint32_t currentBit = 0;
  for (auto &field : m_fields)
    {
      uint32_t bitsToRead = field.bitWidth;
      uint64_t value = 0;

      while (bitsToRead > 0)
        {
          uint32_t byteIndex = currentBit / 8;
          uint32_t bitIndex = currentBit % 8;

          uint32_t freeBits = 8 - bitIndex;
          uint32_t bitsToReadNow = std::min (freeBits, bitsToRead);

          value = (value << bitsToReadNow) |
                  ((buffer[byteIndex] >> (freeBits - bitsToReadNow)) & ((1 << bitsToReadNow) - 1));

          bitsToRead -= bitsToReadNow;
          currentBit += bitsToReadNow;
        }

      field.value = value;
      NS_LOG_DEBUG ("Field " << field.name << ": 0x" << std::hex << std::uppercase << field.value);
    }

  return bytesToRead;
}

uint32_t
CustomHeader::GetSerializedSize (void) const
{
  // Bytes required to store all fields
  uint32_t totalBits = 0;
  for (const auto &field : m_fields)
    {
      totalBits += field.bitWidth;
    }
  return (totalBits + 7) / 8;
}

void
CustomHeader::Print (std::ostream &os) const
{
  os << "CustomHeader { ";
  for (const auto &field : m_fields)
    {
      os << field.name << ": 0x" << std::hex << std::uppercase << field.value << " ";
    }
  os << "}";
}

} // namespace ns3
