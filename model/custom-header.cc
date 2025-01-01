#include "custom-header.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomHeader");

CustomHeader::CustomHeader ()
{
}

CustomHeader::~CustomHeader ()
{
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

TypeId
CustomHeader::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::CustomHeader").SetParent<Header> ().AddConstructor<CustomHeader> ();
  return tid;
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
    }

  return bytesToRead;
}

uint32_t
CustomHeader::GetSerializedSize (void) const
{
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
      os << field.name << ": " << field.value << " ";
    }
  os << "}";
}

} // namespace ns3
