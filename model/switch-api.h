#ifndef SWITCH_API_H
#define SWITCH_API_H

#include <string>
#include <unordered_map>

namespace ns3 {
class SwitchApi {

public:
  // Enums for categorizing APIs
  enum ApiCategory {
    FLOW_TABLE_POPULATION = 0, // APIs for populating flow tables
    FLOW_TABLE_QUERY,          // APIs for querying flow table information
    METER_MANAGEMENT,          // APIs for configuring/querying meters
    COUNTER_OPERATIONS,        // APIs for counter operations
    REGISTER_OPERATIONS,       // APIs for register operations
    INDIRECT_TABLE_ACTION,     // APIs for indirect tables and action profiles
    PARSE_VALUE_SET,           // APIs for parse value sets
    RUNTIME_MANAGEMENT         // APIs for managing runtime state
  };

  // Enums for specific APIs
  enum ApiType {
    // Flow Table Population APIs
    TABLE_SET_DEFAULT = FLOW_TABLE_POPULATION * 100,
    TABLE_ADD,
    TABLE_SET_TIMEOUT,
    TABLE_MODIFY,
    TABLE_DELETE,

    // Meter Management APIs
    METER_ARRAY_SET_RATES = METER_MANAGEMENT * 100,
    METER_SET_RATES,

    // Flow Table Query APIs
    TABLE_NUM_ENTRIES = FLOW_TABLE_QUERY * 100,
    TABLE_CLEAR,
    TABLE_DUMP_ENTRY,
    TABLE_DUMP,

    // Counter Operations
    COUNTER_READ = COUNTER_OPERATIONS * 100,
    COUNTER_RESET,

    // Register Operations
    REGISTER_READ = REGISTER_OPERATIONS * 100,
    REGISTER_WRITE,
    REGISTER_RESET,

    // Indirect Table and Action Profile APIs
    INDIRECT_ADD_ENTRY = INDIRECT_TABLE_ACTION * 100,
    INDIRECT_MODIFY_ENTRY,
    INDIRECT_DELETE_ENTRY,
    INDIRECT_SET_ENTRY_TTL,

    // Parse Value Set APIs
    PARSE_VSET_ADD = PARSE_VALUE_SET * 100,
    PARSE_VSET_REMOVE,
    PARSE_VSET_GET,
    PARSE_VSET_CLEAR,

    // Runtime Management APIs
    RESET_STATE = RUNTIME_MANAGEMENT * 100,
    SERIALIZE,
    LOAD_NEW_CONFIG,
    SWAP_CONFIGS
  };

  static std::unordered_map<std::string, unsigned int> g_apiMap;
  static void InitApiMap();
};

} // namespace ns3

#endif
