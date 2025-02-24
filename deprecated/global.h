#ifndef GLOBAL_H
#define GLOBAL_H

#include "ns3/object.h"
#include <map>
#include <string>
#include <array>
#include <utility>
#include <cstdint>

namespace ns3 {

class P4GlobalVar : public Object
{
public:
  static TypeId GetTypeId (void);

  // Defines the structure and field lengths of the custom header.
  // Users should modify this based on simulation requirements.
  static constexpr std::array<std::pair<const char *, uint32_t>, 2> g_templateHeaderFields = {
      std::make_pair ("field1", 32), std::make_pair ("field2", 64)};

private:
  P4GlobalVar ();
  ~P4GlobalVar ();
  P4GlobalVar (const P4GlobalVar &);
  P4GlobalVar &operator= (const P4GlobalVar &);
};

} // namespace ns3

#endif /* GLOBAL_H */
