#ifndef CUSTOM_HEADER_H
#define CUSTOM_HEADER_H

#include "ns3/header.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace ns3 {

class CustomHeader : public Header
{
public:
  struct Field
  {
    std::string name; // 字段名称
    uint32_t bitWidth; // 字段位宽
    uint64_t value; // 字段值
  };

  CustomHeader ();
  virtual ~CustomHeader ();

  // 添加字段定义
  void AddField (const std::string &name, uint32_t bitWidth);

  // 设置字段值
  void SetField (const std::string &name, uint64_t value);

  // 获取字段值
  uint64_t GetField (const std::string &name) const;

  // 必须实现的 NS-3 方法
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const override;

  virtual void Serialize (Buffer::Iterator start) const override;
  virtual uint32_t Deserialize (Buffer::Iterator start) override;
  virtual uint32_t GetSerializedSize (void) const override;
  virtual void Print (std::ostream &os) const override;

private:
  std::vector<Field> m_fields; // 动态字段列表
};

} // namespace ns3

#endif // CUSTOM_HEADER_H
