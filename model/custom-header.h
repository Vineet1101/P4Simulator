#ifndef CUSTOM_HEADER_H
#define CUSTOM_HEADER_H

#include "ns3/header.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace ns3 {

enum HeaderLayer {
  LAYER_2 = 2, // Data Link Layer
  LAYER_3 = 3, // Network Layer
  LAYER_4 = 4, // Transport Layer
  LAYER_5 = 5 // Application or custom user-defined Layer
};

enum HeaderLayerOperator {
  ADD_BEFORE = 1, // add before the current header exsit in this layer
  REPLACE = 2, // replace the current header exsit in this layer
  ADD_AFTER = 3 // add after the current header exsit in this layer
};

class CustomHeader : public Header
{
public:
  struct Field
  {
    std::string name; // Field name
    uint32_t bitWidth; // Field bit-width
    uint64_t value; // Field value
  };

  CustomHeader ();
  virtual ~CustomHeader ();

  // Add a field definition
  void AddField (const std::string &name, uint32_t bitWidth);

  // Set a field value
  void SetField (const std::string &name, uint64_t value);

  // Set protocol number
  void SetProtocolFieldNumber (uint64_t id);

  // Get protocol number
  uint64_t GetProtocolNumber ();

  // Get a field value
  uint64_t GetField (const std::string &name) const;

  // Set and get header layer
  void SetLayer (HeaderLayer layer);
  HeaderLayer GetLayer () const;

  // Set and get header operator
  void SetOperator (HeaderLayerOperator op);
  HeaderLayerOperator GetOperator () const;

  // NS-3 required methods
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const override;

  virtual void Serialize (Buffer::Iterator start) const override;
  virtual uint32_t Deserialize (Buffer::Iterator start) override;
  virtual uint32_t GetSerializedSize (void) const override;
  virtual void Print (std::ostream &os) const override;

private:
  HeaderLayer m_layer; // OSI Layer for this header
  HeaderLayerOperator m_op; // Operator for this header
  uint64_t m_protocol_number; // Protocol number
  std::vector<Field> m_fields; // Dynamic list of fields
};

} // namespace ns3

#endif // CUSTOM_HEADER_H