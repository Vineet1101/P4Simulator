# RuntimeInterface from BMv2

Below is a categorized description of the APIs from BMv2 source code, organized based on the provided comments and functionality. 
The classification follows the logical groups outlined in the code and adds concise API descriptions for each function.

PS: This is gene by ChatGPT

---

### **API Categories Based on Code Structure**

#### 1. **Flow Table Operations**
APIs related to flow table management: adding, modifying, deleting entries, and configuring default actions or timeouts.

| **API**                        | **Description**                                                                                   |
|--------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_get_num_entries`           | Retrieves the number of entries in a match table.                                                |
| `mt_clear_entries`             | Clears all entries in a match table, with an option to reset the default entry.                  |
| `mt_add_entry`                 | Adds an entry to a match table with a specified match key, action, and optional priority.         |
| `mt_set_default_action`        | Sets the default action for a match table.                                                       |
| `mt_reset_default_entry`       | Resets the default entry for a match table.                                                      |
| `mt_delete_entry`              | Deletes a specific entry in a match table.                                                       |
| `mt_modify_entry`              | Modifies an existing match table entry with a new action and parameters.                         |
| `mt_set_entry_ttl`             | Sets a timeout (TTL) in milliseconds for a specific table entry.                                 |

---

#### 2. **Action Profile Operations**
APIs for managing action profiles: adding members, modifying them, and creating or managing groups.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_act_prof_add_member`               | Adds a member to an action profile with a specific action and data.                              |
| `mt_act_prof_delete_member`            | Deletes a member from an action profile.                                                         |
| `mt_act_prof_modify_member`            | Modifies an action profile member's action and parameters.                                       |
| `mt_act_prof_create_group`             | Creates a group in an action profile.                                                            |
| `mt_act_prof_delete_group`             | Deletes a group in an action profile.                                                            |
| `mt_act_prof_add_member_to_group`      | Adds a member to a specific group in an action profile.                                          |
| `mt_act_prof_remove_member_from_group` | Removes a member from a group in an action profile.                                              |
| `mt_act_prof_get_members`              | Retrieves all members in an action profile.                                                      |
| `mt_act_prof_get_member`               | Retrieves a specific member from an action profile.                                              |
| `mt_act_prof_get_groups`               | Retrieves all groups in an action profile.                                                       |
| `mt_act_prof_get_group`                | Retrieves a specific group in an action profile.                                                 |

---

#### 3. **Indirect Table Operations**
APIs for indirect tables, including management of entries and default actions.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_indirect_add_entry`                | Adds an entry to an indirect match table with a member handle.                                    |
| `mt_indirect_modify_entry`             | Modifies an entry in an indirect match table with a new member handle.                           |
| `mt_indirect_delete_entry`             | Deletes an entry from an indirect match table.                                                   |
| `mt_indirect_set_entry_ttl`            | Sets a timeout (TTL) for an indirect table entry.                                                |
| `mt_indirect_set_default_member`       | Sets the default action or member for an indirect match table.                                   |
| `mt_indirect_reset_default_entry`      | Resets the default entry for an indirect match table.                                            |
| `mt_indirect_ws_add_entry`             | Adds an entry to an indirect wide-switch match table with a group handle.                        |
| `mt_indirect_ws_modify_entry`          | Modifies an entry in an indirect wide-switch match table with a new group handle.                |
| `mt_indirect_ws_set_default_group`     | Sets the default group for an indirect wide-switch match table.                                  |

---

#### 4. **Flow Table Entry Retrieval**
APIs to query entries or defaults in flow tables.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_get_entries`                       | Retrieves all entries from a match table.                                                        |
| `mt_indirect_get_entries`              | Retrieves all entries from an indirect match table.                                              |
| `mt_indirect_ws_get_entries`           | Retrieves all entries from an indirect wide-switch match table.                                  |
| `mt_get_entry`                         | Retrieves a specific entry from a match table by handle.                                         |
| `mt_indirect_get_entry`                | Retrieves a specific entry from an indirect match table by handle.                               |
| `mt_indirect_ws_get_entry`             | Retrieves a specific entry from an indirect wide-switch match table by handle.                   |
| `mt_get_default_entry`                 | Retrieves the default entry from a match table.                                                  |
| `mt_indirect_get_default_entry`        | Retrieves the default entry from an indirect match table.                                        |
| `mt_indirect_ws_get_default_entry`     | Retrieves the default entry from an indirect wide-switch match table.                            |
| `mt_get_entry_from_key`                | Retrieves a match table entry based on a match key.                                              |
| `mt_indirect_get_entry_from_key`       | Retrieves an indirect match table entry based on a match key.                                    |
| `mt_indirect_ws_get_entry_from_key`    | Retrieves an indirect wide-switch match table entry based on a match key.                        |

---

#### 5. **Counter Operations**
APIs to query or modify counters.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_read_counters`                     | Reads counter values (bytes and packets) for a specific table entry.                             |
| `mt_reset_counters`                    | Resets all counters in a match table.                                                            |
| `mt_write_counters`                    | Writes counter values (bytes and packets) for a specific table entry.                            |
| `read_counters`                        | Reads counter values (bytes and packets) for a specific counter.                                 |
| `reset_counters`                       | Resets a specific counter.                                                                       |
| `write_counters`                       | Writes counter values (bytes and packets) to a specific counter.                                 |

---

#### 6. **Meter Operations**
APIs to configure or query meter rates.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `mt_set_meter_rates`                   | Configures meter rates for a specific table entry.                                               |
| `mt_get_meter_rates`                   | Retrieves meter rates for a specific table entry.                                                |
| `mt_reset_meter_rates`                 | Resets meter rates for a specific table entry.                                                   |
| `meter_array_set_rates`                | Configures rates for an entire meter array.                                                      |
| `meter_set_rates`                      | Configures rates for a specific meter.                                                           |
| `meter_get_rates`                      | Retrieves rates for a specific meter.                                                            |
| `meter_reset_rates`                    | Resets rates for a specific meter.                                                               |

---

#### 7. **Register Operations**
APIs to read, write, or reset registers.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `register_read`                        | Reads the value of a specific register.                                                          |
| `register_read_all`                    | Reads values of all cells in a register array.                                                   |
| `register_write`                       | Writes a value to a specific register.                                                           |
| `register_write_range`                 | Writes a value to a range of registers.                                                          |
| `register_reset`                       | Resets all values in a register array.                                                           |

---

#### 8. **Parse Value Set Operations**
APIs to manage parse value sets.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `parse_vset_add`                       | Adds a value to a parse value set.                                                               |
| `parse_vset_remove`                    | Removes a value from a parse value set.                                                          |
| `parse_vset_get`                       | Retrieves all values in a parse value set.                                                       |
| `parse_vset_clear`                     | Clears all values in a parse value set.                                                          |

---

#### 9. **Runtime State Management**
APIs for managing runtime configuration and state.

| **API**                                | **Description**                                                                                   |
|----------------------------------------|---------------------------------------------------------------------------------------------------|
| `reset_state`                          | Resets the runtime state to its initial configuration.                                           |
| `serialize`                            | Serializes the runtime state to an output stream.                                                |
| `load_new_config`                      | Loads a new runtime configuration.                                                              |
| `swap_configs`                         | Swaps the current configuration with a newly loaded one.                                         |
| `get_config`                           | Retrieves the current runtime configuration as a string.                                         |
| `get_config_md5`                       | Retrieves the MD5 hash of the current runtime configuration.                                     |

---

