#include "switch-api.h"

namespace ns3 {

// Define the static member
std::unordered_map<std::string, unsigned int> SwitchApi::g_apiMap;

void SwitchApi::InitApiMap() {
  // Populate Flow Table
  g_apiMap["table_set_default"] = TABLE_SET_DEFAULT;
  g_apiMap["table_add"] = TABLE_ADD;
  g_apiMap["table_set_timeout"] = TABLE_SET_TIMEOUT;
  g_apiMap["table_modify"] = TABLE_MODIFY;
  g_apiMap["table_delete"] = TABLE_DELETE;

  // Meter Management
  g_apiMap["meter_array_set_rates"] = METER_ARRAY_SET_RATES;
  g_apiMap["meter_set_rates"] = METER_SET_RATES;

  // Flow Table Query
  g_apiMap["table_num_entries"] = TABLE_NUM_ENTRIES;
  g_apiMap["table_clear"] = TABLE_CLEAR;
  g_apiMap["table_dump_entry"] = TABLE_DUMP_ENTRY;
  g_apiMap["table_dump"] = TABLE_DUMP;

  // Counter Operations
  g_apiMap["counter_read"] = COUNTER_READ;
  g_apiMap["counter_reset"] = COUNTER_RESET;

  // Register Operations
  g_apiMap["register_read"] = REGISTER_READ;
  g_apiMap["register_write"] = REGISTER_WRITE;
  g_apiMap["register_reset"] = REGISTER_RESET;

  // Indirect Table and Action Profile APIs
  g_apiMap["indirect_add_entry"] = INDIRECT_ADD_ENTRY;
  g_apiMap["indirect_modify_entry"] = INDIRECT_MODIFY_ENTRY;
  g_apiMap["indirect_delete_entry"] = INDIRECT_DELETE_ENTRY;
  g_apiMap["indirect_set_entry_ttl"] = INDIRECT_SET_ENTRY_TTL;

  // Parse Value Set APIs
  g_apiMap["parse_vset_add"] = PARSE_VSET_ADD;
  g_apiMap["parse_vset_remove"] = PARSE_VSET_REMOVE;
  g_apiMap["parse_vset_get"] = PARSE_VSET_GET;
  g_apiMap["parse_vset_clear"] = PARSE_VSET_CLEAR;

  // Runtime Management APIs
  g_apiMap["reset_state"] = RESET_STATE;
  g_apiMap["serialize"] = SERIALIZE;
  g_apiMap["load_new_config"] = LOAD_NEW_CONFIG;
  g_apiMap["swap_configs"] = SWAP_CONFIGS;
}
} // namespace ns3
