# Runtime APIs for the P4 Switch

Here is a categorized and summarized list of the runtime APIs.

---

## Summary
This classification organizes the APIs into functional groups:
1. **Flow Table APIs** (population and query).
2. **Meter Management**.
3. **Counter Operations**.
4. **Register Operations**.
5. **Indirect Table and Action Profile APIs**.
6. **Parse Value Sets**.
7. **Runtime Management**.

### 1. **Flow Table Population APIs**
These APIs are used to modify or manage entries in the flow table.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **TABLE_SET_DEFAULT**         | Set the default action for a match table.                                                               | `table_set_default`        |
| **TABLE_ADD**                 | Add an entry to a match table.                                                                          | `table_add`                |
| **TABLE_MODIFY**              | Modify an entry in a match table.                                                                       | `table_modify`             |
| **TABLE_DELETE**              | Delete an entry from a match table.                                                                     | `table_delete`             |
| **TABLE_SET_TIMEOUT**         | Set a timeout in milliseconds for a given entry in a table.                                             | `table_set_timeout`        |

---

### 2. **Flow Table Query APIs**
These APIs retrieve information about the flow table or its entries.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **TABLE_NUM_ENTRIES**         | Return the number of entries in a match table.                                                          | `table_num_entries`        |
| **TABLE_CLEAR**               | Clear all entries in a match table (does not reset the default entry).                                  | `table_clear`              |
| **TABLE_DUMP_ENTRY**          | Display information about a specific table entry.                                                       | `table_dump_entry`         |
| **TABLE_DUMP**                | Display all entries in a match table.                                                                   | `table_dump`               |

---

### 3. **Meter Configuration and Query APIs**
These APIs manage or query the behavior of meters.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **METER_ARRAY_SET_RATES**     | Configure rates for an entire meter array.                                                              | `meter_array_set_rates`    |
| **METER_SET_RATES**           | Configure rates for a specific meter.                                                                   | `meter_set_rates`          |
| **METER_GET_RATES**           | Retrieve rates for a specific meter.                                                                    | `meter_get_rates`          |

---

### 4. **Counter APIs**
These APIs manage or query counters.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **COUNTER_READ**              | Read the value of a counter.                                                                            | `counter_read`             |
| **COUNTER_RESET**             | Reset a counter to its initial state.                                                                   | `counter_reset`            |

---

### 5. **Register APIs**
These APIs manage or query registers.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **REGISTER_READ**             | Read a value from a register at a given index.                                                          | `register_read`            |
| **REGISTER_WRITE**            | Write a value to a register at a given index.                                                           | `register_write`           |
| **REGISTER_RESET**            | Reset all cells in a register array to `0`.                                                             | `register_reset`           |

---

### 6. **Indirect Table and Action Profile APIs**
These APIs handle indirect match tables and action profiles.

| **API Name**                         | **Description**                                                                                     | **Command**                     |
|--------------------------------------|-----------------------------------------------------------------------------------------------------|---------------------------------|
| **mt_indirect_add_entry**            | Add an entry to an indirect match table.                                                           | `indirect_add_entry`           |
| **mt_indirect_modify_entry**         | Modify an entry in an indirect match table.                                                        | `indirect_modify_entry`        |
| **mt_indirect_delete_entry**         | Delete an entry from an indirect match table.                                                      | `indirect_delete_entry`        |
| **mt_indirect_set_entry_ttl**        | Set a TTL for an indirect table entry.                                                             | `indirect_set_entry_ttl`       |
| **mt_indirect_set_default_member**   | Set the default action/member for an indirect table.                                               | `indirect_set_default_member`  |
| **mt_act_prof_add_member**           | Add a member to an action profile.                                                                 | `act_prof_add_member`          |
| **mt_act_prof_delete_member**        | Delete a member from an action profile.                                                            | `act_prof_delete_member`       |
| **mt_act_prof_modify_member**        | Modify a member in an action profile.                                                              | `act_prof_modify_member`       |
| **mt_act_prof_create_group**         | Create a group in an action profile.                                                               | `act_prof_create_group`        |
| **mt_act_prof_delete_group**         | Delete a group in an action profile.                                                               | `act_prof_delete_group`        |
| **mt_act_prof_add_member_to_group**  | Add a member to a group in an action profile.                                                      | `act_prof_add_member_to_group` |
| **mt_act_prof_remove_member_from_group** | Remove a member from a group in an action profile.                                               | `act_prof_remove_member_from_group` |

---

### 7. **Parsing Value Set APIs**
These APIs manage parse value sets.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **parse_vset_add**            | Add a value to a parse value set.                                                                       | `parse_vset_add`           |
| **parse_vset_remove**         | Remove a value from a parse value set.                                                                  | `parse_vset_remove`        |
| **parse_vset_get**            | Retrieve all values in a parse value set.                                                              | `parse_vset_get`           |
| **parse_vset_clear**          | Clear all values in a parse value set.                                                                  | `parse_vset_clear`         |

---

### 8. **Runtime State Management APIs**
These APIs manage the runtime state of the system.

| **API Name**                  | **Description**                                                                                          | **Command**                |
|-------------------------------|----------------------------------------------------------------------------------------------------------|----------------------------|
| **reset_state**               | Reset the runtime state.                                                                                | `reset_state`             |
| **serialize**                 | Serialize the runtime state to an output stream.                                                       | `serialize`               |
| **load_new_config**           | Load a new configuration into the runtime.                                                             | `load_new_config`         |
| **swap_configs**              | Swap the current configuration with the new one.                                                       | `swap_configs`            |

---


