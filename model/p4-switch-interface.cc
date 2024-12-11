#include "ns3/p4-switch-interface.h"
#include "ns3/format-utils.h"
#include "ns3/p4-exception-handle.h"

#include "ns3/log.h"

#include <bm/bm_sim/options_parse.h>
#include <bm/bm_runtime/bm_runtime.h>
#include <bm/bm_sim/switch.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4SwitchInterface");

NS_OBJECT_ENSURE_REGISTERED (P4SwitchInterface);

TypeId
P4SwitchInterface::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::P4SwitchInterface").SetParent<Object> ().SetGroupName ("P4SwitchInterface");
  return tid;
}

P4SwitchInterface::P4SwitchInterface ()
{
  NS_LOG_FUNCTION (this);
}

P4SwitchInterface::~P4SwitchInterface ()
{
  //********TO DO (Whether need delete p4Core_)*******************
  /*if(p4Core_!=nullptr)
    {
        delete p4Core_;
        p4Core_=nullptr;
    }*/
  //***************************************************************
  NS_LOG_FUNCTION (this);
}

void
P4SwitchInterface::PopulateFlowTable ()
{
  // Attempt to open the flow table file at the specified path
  std::ifstream fileStream (flowTablePath_);
  if (!fileStream.is_open ())
    {
      NS_LOG_ERROR ("Unable to open flow table file: " << flowTablePath_);
      return;
    }

  // const int maxBufferSize = 500; // Define the maximum buffer size for reading rows
  std::string line; // String to hold each line from the file

  NS_LOG_INFO ("Starting to populate the flow table from: " << flowTablePath_);

  // Read the file line by line
  while (std::getline (fileStream, line))
    {
      // Process each line to populate the flow table
      if (!line.empty ())
        {
          NS_LOG_DEBUG ("Processing line: " << line);
          ParsePopulateFlowTableCommand (line);
        }
    }

  NS_LOG_INFO ("Finished populating the flow table.");
  // Close the file stream (automatically handled by RAII)
  fileStream.close ();
}

void
P4SwitchInterface::ReadP4Info ()
{
  NS_LOG_INFO ("Reading P4Info from file: " << p4InfoPath_);

  // Opening a file
  std::ifstream topgen (p4InfoPath_);
  if (!topgen.is_open ())
    {
      NS_LOG_ERROR ("Failed to open P4Info file: " << p4InfoPath_);
      abort (); // Terminate the program when the file cannot be opened
    }

  std::string line;
  std::istringstream lineBuffer;
  std::string elementType;

  std::string tableName, matchType, meterName, counterName;
  unsigned int isDirect;

  while (std::getline (topgen, line))
    {
      lineBuffer.clear ();
      lineBuffer.str (line);

      // Parsing the first item: elementType
      lineBuffer >> elementType;

      if (elementType == "table")
        {
          lineBuffer >> tableName >> matchType;
          if (matchType == "exact")
            {
              flowTables_[tableName].matchType = bm::MatchKeyParam::Type::EXACT;
            }
          else if (matchType == "lpm")
            {
              flowTables_[tableName].matchType = bm::MatchKeyParam::Type::LPM;
            }
          else if (matchType == "ternary")
            {
              flowTables_[tableName].matchType = bm::MatchKeyParam::Type::TERNARY;
            }
          else if (matchType == "valid")
            {
              flowTables_[tableName].matchType = bm::MatchKeyParam::Type::VALID;
            }
          else if (matchType == "range")
            {
              flowTables_[tableName].matchType = bm::MatchKeyParam::Type::RANGE;
            }
          else
            {
              NS_LOG_ERROR ("Undefined match type in table: " << matchType);
            }
        }
      else if (elementType == "meter")
        {
          lineBuffer >> meterName >> isDirect >> tableName;
          meters_[meterName].tableName = tableName;
          meters_[meterName].isDirect = (isDirect == 1);

          NS_LOG_DEBUG ("Parsed meter: " << meterName
                                         << " (isDirect=" << meters_[meterName].isDirect
                                         << ", tableName=" << tableName << ")");
        }
      else if (elementType == "counter")
        {
          lineBuffer >> counterName >> isDirect >> tableName;
          counters_[counterName].isDirect = (isDirect == 1);
          counters_[counterName].tableName = tableName;

          NS_LOG_DEBUG ("Parsed counter: " << counterName
                                           << " (isDirect=" << counters_[counterName].isDirect
                                           << ", tableName=" << tableName << ")");
        }
      else
        {
          NS_LOG_WARN ("Undefined element type in P4Info: " << elementType);
        }
    }

  NS_LOG_INFO ("Finished reading P4Info from file: " << p4InfoPath_);
}

void
P4SwitchInterface::ViewFlowtableEntryNum ()
{
  NS_LOG_INFO ("Viewing the number of entries for each flow table.");

  // Iterate over the flowTables_ map
  for (auto iter = flowTables_.begin (); iter != flowTables_.end (); ++iter)
    {
      const std::string &tableName = iter->first; // Get the table name
      std::string command = "table_num_entries " + tableName;

      NS_LOG_DEBUG ("Sending command to retrieve entry count for table: " << tableName);

      // Execute the command to get table entry information
      ParseAttainFlowTableInfoCommand (command);

      // If needed, replace with actual functionality to retrieve the entry count
      // Example:
      // size_t numEntries = 0;
      // mt_get_num_entries(0, tableName, &numEntries);
      // NS_LOG_INFO("Flow table '" << tableName << "' has " << numEntries << " entries.");
    }

  NS_LOG_INFO ("Finished viewing flow table entry numbers.");
}

void
P4SwitchInterface::AttainSwitchFlowTableInfo ()
{
  NS_LOG_INFO (
      "Starting to attain switch flow table information from file: " << viewFlowTablePath_);

  // Open the file at the specified path
  std::ifstream fileStream (viewFlowTablePath_);
  if (!fileStream.is_open ())
    {
      NS_LOG_ERROR ("Failed to open file: " << viewFlowTablePath_);
      return;
    }

  std::string line;

  // Read the file line by line
  while (std::getline (fileStream, line))
    {
      if (!line.empty ())
        {
          NS_LOG_DEBUG ("Processing command: " << line);
          ParseAttainFlowTableInfoCommand (line); // Process the command
        }
    }

  NS_LOG_INFO (
      "Finished processing switch flow table information from file: " << viewFlowTablePath_);
}

void
P4SwitchInterface::ParseAttainFlowTableInfoCommand (const std::string &commandRow)
{
  NS_LOG_INFO ("Processing command: " << commandRow);

  // Split the command row into parameters
  std::vector<std::string> parms;
  size_t lastPos = 0, currentPos = 0;
  while ((currentPos = commandRow.find (' ', lastPos)) != std::string::npos)
    {
      parms.emplace_back (commandRow.substr (lastPos, currentPos - lastPos));
      lastPos = currentPos + 1;
    }
  if (lastPos < commandRow.size ())
    {
      parms.emplace_back (commandRow.substr (lastPos));
    }

  // Ensure we have at least one parameter to process
  if (parms.empty ())
    {
      NS_LOG_WARN ("Empty command row received.");
      return;
    }

  // Retrieve the command type
  unsigned int commandType = 0;
  try
    {
      commandType = SwitchApi::g_apiMap.at (parms[0]);
    }
  catch (const std::out_of_range &)
    {
      NS_LOG_ERROR ("Invalid command type: " << parms[0]);
      return;
    }

  try
    {
      // Ensure p4Core_ is initialized
      if (p4Core_ == nullptr)
        {
          throw P4Exception (P4ErrorCode::P4_SWITCH_POINTER_NULL);
        }

      // Process the command based on the command type
      switch (commandType)
        {
        case SwitchApi::ApiType::MT_GET_NUM_ENTRIES:
          HandleTableNumEntries (parms);
          break;

        case SwitchApi::ApiType::MT_CLEAR_ENTRIES:
          HandleTableClear (parms);
          break;

        case SwitchApi::ApiType::METER_GET_RATES:
          HandleMeterGetRates (parms);
          break;

        case SwitchApi::ApiType::READ_COUNTERS:
          HandleCounterRead (parms);
          break;

        case SwitchApi::ApiType::RESET_COUNTERS:
          HandleCounterReset (parms);
          break;

        case SwitchApi::ApiType::REGISTER_READ:
          HandleRegisterRead (parms);
          break;

        case SwitchApi::ApiType::REGISTER_WRITE:
          HandleRegisterWrite (parms);
          break;

        case SwitchApi::ApiType::REGISTER_RESET:
          HandleRegisterReset (parms);
          break;

        case SwitchApi::ApiType::MT_GET_ENTRY:
          HandleTableDumpEntry (parms);
          break;

        case SwitchApi::ApiType::MT_GET_ENTRIES:
          HandleTableDump (parms);
          break;

        default:
          throw P4Exception (P4ErrorCode::COMMAND_ERROR);
        }
    }
  catch (P4Exception &e)
    {
      NS_LOG_ERROR ("P4Exception caught: " << e.what ());
      e.ShowExceptionEntry (e.info ());
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Standard exception caught: " << e.what ());
      P4Exception fallbackException (P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
      fallbackException.ShowExceptionEntry ("Standard exception: " + std::string (e.what ()));
    }
  catch (...)
    {
      NS_LOG_ERROR ("Unknown exception caught.");
      P4Exception fallbackException (P4ErrorCode::OTHER_ERROR, "Unknown exception");
      fallbackException.ShowExceptionEntry (commandRow);
    }
}

void
P4SwitchInterface::HandleTableDumpEntry (const std::vector<std::string> &parms)
{
  if (parms.size () != 3)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for TABLE_DUMP_ENTRY");
    }

  bm::entry_handle_t handle (StrToInt (parms[2])); // Parse the entry handle
  bm::MatchTable::Entry entry;

  // Retrieve the entry
  if (p4Core_->mt_get_entry (0, parms[1], handle, &entry) != bm::MatchErrorCode::SUCCESS)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS,
                         "Failed to retrieve table entry for table: " + parms[1]);
    }

  // Display the entry information
  std::ostringstream oss;
  oss << parms[1] << " entry " << handle << " :\nMatchKey: ";
  for (const auto &key : entry.match_key)
    {
      oss << key.key << " ";
    }
  oss << "\nActionData: ";
  for (const auto &data : entry.action_data.action_data)
    {
      oss << data << " ";
    }

  NS_LOG_INFO (oss.str ());
}

void
P4SwitchInterface::HandleTableNumEntries (const std::vector<std::string> &parms)
{
  if (parms.size () != 2)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for TABLE_NUM_ENTRIES");
    }

  size_t numEntries = 0;

  // Retrieve the number of entries
  if (p4Core_->mt_get_num_entries (0, parms[1], &numEntries) != bm::MatchErrorCode::SUCCESS)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS,
                         "Failed to retrieve table entries for table: " + parms[1]);
    }

  NS_LOG_INFO ("Table " << parms[1] << " has " << numEntries << " entries.");
}

void
P4SwitchInterface::HandleTableClear (const std::vector<std::string> &parms)
{
  if (parms.size () != 2)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for TABLE_CLEAR");
    }

  // Clear the table
  if (p4Core_->mt_clear_entries (0, parms[1], false) != bm::MatchErrorCode::SUCCESS)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS, "Failed to clear entries in table: " + parms[1]);
    }

  NS_LOG_INFO ("Successfully cleared entries in table: " << parms[1]);
}

void
P4SwitchInterface::HandleMeterGetRates (const std::vector<std::string> &parms)
{
  if (parms.size () != 3)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for METER_GET_RATES");
    }

  if (meters_.count (parms[1]) == 0)
    {
      throw P4Exception (P4ErrorCode::METER_NO_EXIST, "Meter does not exist: " + parms[1]);
    }

  const auto &meter = meters_[parms[1]];
  std::vector<bm::Meter::rate_config_t> configs;

  if (meter.isDirect)
    {
      bm::entry_handle_t handle (StrToInt (parms[2]));
      if (p4Core_->mt_get_meter_rates (0, meter.tableName, handle, &configs) !=
          bm::MatchErrorCode::SUCCESS)
        {
          throw P4Exception (P4ErrorCode::NO_SUCCESS,
                             "Failed to retrieve meter rates for direct meter: " + parms[1]);
        }
    }
  else
    {
      size_t index = StrToInt (parms[2]);
      if (p4Core_->meter_get_rates (0, parms[1], index, &configs) != 0)
        {
          throw P4Exception (P4ErrorCode::NO_SUCCESS,
                             "Failed to retrieve meter rates for indirect meter: " + parms[1]);
        }
    }

  for (const auto &config : configs)
    {
      NS_LOG_INFO ("Meter " << parms[1] << " RateConfig: info_rate=" << config.info_rate
                            << ", burst_size=" << config.burst_size);
    }
}

void
P4SwitchInterface::HandleCounterRead (const std::vector<std::string> &parms)
{
  if (parms.size () != 3)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for COUNTER_READ");
    }

  if (counters_.count (parms[1]) == 0)
    {
      throw P4Exception (P4ErrorCode::COUNTER_NO_EXIST, "Counter does not exist: " + parms[1]);
    }

  const auto &counter = counters_[parms[1]];
  bm::MatchTableAbstract::counter_value_t bytes = 0, packets = 0;

  if (counter.isDirect)
    {
      bm::entry_handle_t handle (StrToInt (parms[2]));
      if (p4Core_->mt_read_counters (0, counter.tableName, handle, &bytes, &packets) !=
          bm::MatchErrorCode::SUCCESS)
        {
          throw P4Exception (P4ErrorCode::NO_SUCCESS,
                             "Failed to read counter for direct counter: " + parms[1]);
        }
    }
  else
    {
      size_t index = StrToInt (parms[2]);
      if (p4Core_->read_counters (0, parms[1], index, &bytes, &packets) != 0)
        {
          throw P4Exception (P4ErrorCode::NO_SUCCESS,
                             "Failed to read counter for indirect counter: " + parms[1]);
        }
    }

  NS_LOG_INFO ("Counter " << parms[1] << " [" << parms[2] << "]: " << bytes << " bytes, " << packets
                          << " packets.");
}

void
P4SwitchInterface::HandleCounterReset (const std::vector<std::string> &parms)
{
  // @TODO (Might need to change the parameter list)
  return;
}

void
P4SwitchInterface::HandleRegisterRead (const std::vector<std::string> &parms)
{
  if (parms.size () != 3)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for REGISTER_READ");
    }

  size_t index = StrToInt (parms[2]);
  bm::Data value;

  // Read the register value
  if (p4Core_->register_read (0, parms[1], index, &value) != 0)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS,
                         "Failed to read register: " + parms[1] + " at index " + parms[2]);
    }

  NS_LOG_INFO ("Register " << parms[1] << "[" << index << "] value: " << value);
}

void
P4SwitchInterface::HandleRegisterWrite (const std::vector<std::string> &parms)
{
  if (parms.size () != 4)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for REGISTER_WRITE");
    }

  size_t index = StrToInt (parms[2]);
  bm::Data value (parms[3]);

  // Write the value to the register
  if (p4Core_->register_write (0, parms[1], index, value) != 0)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS, "Failed to write to register: " + parms[1] +
                                                      " at index " + parms[2] + " with value " +
                                                      parms[3]);
    }

  NS_LOG_INFO ("Successfully wrote value " << value << " to register " << parms[1] << "[" << index
                                           << "].");
}

void
P4SwitchInterface::HandleRegisterReset (const std::vector<std::string> &parms)
{
  if (parms.size () != 2)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for REGISTER_RESET");
    }

  // Reset the register
  if (p4Core_->register_reset (0, parms[1]) != 0)
    {
      throw P4Exception (P4ErrorCode::NO_SUCCESS, "Failed to reset register: " + parms[1]);
    }

  NS_LOG_INFO ("Successfully reset register: " << parms[1]);
}

void
P4SwitchInterface::HandleTableDump (const std::vector<std::string> &parms)
{
  if (parms.size () != 2)
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                         "Invalid number of parameters for TABLE_DUMP");
    }

  std::vector<bm::MatchTable::Entry> entries;

  // Retrieve all entries from the table
  entries = p4Core_->mt_get_entries (0, parms[1]);

  if (entries.empty ())
    {
      NS_LOG_INFO ("Table " << parms[1] << " has no entries.");
      return;
    }

  // Log each entry's details
  std::ostringstream oss;
  oss << "Dumping entries for table: " << parms[1] << "\n";
  for (const auto &entry : entries)
    {
      oss << "Entry:\n  MatchKey: ";
      for (const auto &key : entry.match_key)
        {
          oss << key.key << " ";
        }
      oss << "\n  ActionData: ";
      for (const auto &data : entry.action_data.action_data)
        {
          oss << data << " ";
        }
      oss << "\n";
    }

  NS_LOG_INFO (oss.str ());
}

void
P4SwitchInterface::ParsePopulateFlowTableCommand (const std::string &commandRow)
{
  std::vector<std::string> parms;
  int lastP = 0, curP = 0;

  // Parse the command row into parameters
  for (size_t i = 0; i < commandRow.size (); i++, curP++)
    {
      if (commandRow[i] == ' ')
        {
          parms.push_back (commandRow.substr (lastP, curP - lastP));
          lastP = i + 1;
        }
    }
  if (lastP < curP)
    {
      parms.push_back (commandRow.substr (lastP, curP - lastP));
    }

  // Ensure parameters are present
  if (parms.empty ())
    {
      NS_LOG_ERROR ("Command row is empty. No operation performed.");
      return;
    }

  // Get command type
  unsigned int commandType = SwitchApi::g_apiMap[parms[0]];

  // Exception handling for the switch block
  try
    {
      if (p4Core_ == nullptr)
        throw P4Exception (P4ErrorCode::P4_SWITCH_POINTER_NULL);

      // Execute command-specific logic
      switch (commandType)
        {
        case SwitchApi::ApiType::MT_SET_DEFAULT_ACTION:
          HandleTableSetDefault (parms);
          break;
        case SwitchApi::ApiType::MT_ADD_ENTRY:
          HandleTableAdd (parms);
          break;
        case SwitchApi::ApiType::MT_SET_ENTRY_TTL:
          HandleTableSetTimeout (parms);
          break;
        case SwitchApi::ApiType::MT_MODIFY_ENTRY:
          HandleTableModify (parms);
          break;
        case SwitchApi::ApiType::MT_DELETE_ENTRY:
          HandleTableDelete (parms);
          break;
        case SwitchApi::ApiType::METER_ARRAY_SET_RATES:
          HandleMeterArraySetRates (parms);
          break;
        case SwitchApi::ApiType::METER_SET_RATES:
          HandleMeterSetRates (parms);
          break;
        default:
          throw P4Exception (P4ErrorCode::COMMAND_ERROR);
        }
    }
  catch (P4Exception &e)
    {
      // Unified exception handling for P4 exceptions
      NS_LOG_ERROR ("P4Exception caught: " << e.what ());
      e.ShowExceptionEntry (e.info ());
    }
  catch (const std::exception &e)
    {
      // Handle standard exceptions
      NS_LOG_ERROR ("Standard exception caught: " << e.what ());
      P4Exception fallbackException (P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
      fallbackException.ShowExceptionEntry ("Standard exception: " + std::string (e.what ()));
    }
  catch (...)
    {
      // Handle unknown exceptions
      NS_LOG_ERROR ("Unknown exception caught.");
      P4Exception fallbackException (P4ErrorCode::OTHER_ERROR, "Unknown exception");
      fallbackException.ShowExceptionEntry (commandRow);
    }
}

void
P4SwitchInterface::HandleTableSetDefault (const std::vector<std::string> &parms)
{
  if (parms.size () >= 3)
    {
      bm::ActionData actionData;
      for (size_t i = 3; i < parms.size (); i++)
        {
          actionData.push_back_action_data (bm::Data (parms[i]));
        }
      if (p4Core_->mt_set_default_action (0, parms[1], parms[2], actionData) !=
          bm::MatchErrorCode::SUCCESS)
        {
          throw P4Exception (P4ErrorCode::NO_SUCCESS);
        }
    }
  else
    {
      throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);
    }
}

void
P4SwitchInterface::HandleTableAdd (const std::vector<std::string> &parms)
{
  if (parms.size () < 4)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  bm::MatchKeyParam::Type matchType = flowTables_[parms[1]].matchType;
  std::vector<bm::MatchKeyParam> matchKey;
  bm::ActionData actionData;
  bm::entry_handle_t handle;
  size_t i = 3;

  // Parse match fields
  for (; i < parms.size () && parms[i] != "=>"; i++)
    {
      matchKey.push_back (ParseMatchField (parms[i], matchType));
    }

  // Skip the "=>" separator
  i++;

  // Parse action parameters
  for (; i < parms.size (); i++)
    {
      actionData.push_back_action_data (bm::Data (parms[i]));
    }

  // Add entry to the match-action table
  if (p4Core_->mt_add_entry (0, parms[1], matchKey, parms[2], actionData, &handle) !=
      bm::MatchErrorCode::SUCCESS)
    throw P4Exception (P4ErrorCode::NO_SUCCESS);
}

void
P4SwitchInterface::HandleTableSetTimeout (const std::vector<std::string> &parms)
{
  if (parms.size () != 4)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  bm::entry_handle_t handle (StrToInt (parms[2]));
  unsigned int ttl_ms = StrToInt (parms[3]);

  if (p4Core_->mt_set_entry_ttl (0, parms[1], handle, ttl_ms) != bm::MatchErrorCode::SUCCESS)
    throw P4Exception (P4ErrorCode::NO_SUCCESS);
}

void
P4SwitchInterface::HandleTableModify (const std::vector<std::string> &parms)
{
  if (parms.size () < 4)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  bm::ActionData actionData;
  bm::entry_handle_t handle (StrToInt (parms[3]));

  for (size_t i = 4; i < parms.size (); i++)
    {
      actionData.push_back_action_data (bm::Data (parms[i]));
    }

  if (p4Core_->mt_modify_entry (0, parms[1], handle, parms[2], actionData) !=
      bm::MatchErrorCode::SUCCESS)
    throw P4Exception (P4ErrorCode::NO_SUCCESS);
}

void
P4SwitchInterface::HandleTableDelete (const std::vector<std::string> &parms)
{
  if (parms.size () != 3)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  bm::entry_handle_t handle (StrToInt (parms[2]));

  if (p4Core_->mt_delete_entry (0, parms[1], handle) != bm::MatchErrorCode::SUCCESS)
    throw P4Exception (P4ErrorCode::NO_SUCCESS);
}

void
P4SwitchInterface::HandleMeterArraySetRates (const std::vector<std::string> &parms)
{
  if (parms.size () <= 2)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  std::vector<bm::Meter::rate_config_t> configs;

  for (size_t i = 2; i < parms.size (); i++)
    {
      configs.push_back (ParseRateConfig (parms[i]));
    }

  if (p4Core_->meter_array_set_rates (0, parms[1], configs) != 0)
    throw P4Exception (P4ErrorCode::NO_SUCCESS);
}

void
P4SwitchInterface::HandleMeterSetRates (const std::vector<std::string> &parms)
{
  if (parms.size () <= 3)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR);

  std::vector<bm::Meter::rate_config_t> configs;

  for (size_t i = 3; i < parms.size (); i++)
    {
      configs.push_back (ParseRateConfig (parms[i]));
    }

  if (meters_[parms[1]].isDirect) // Direct meter
    {
      bm::entry_handle_t handle (StrToInt (parms[2]));
      if (p4Core_->mt_set_meter_rates (0, meters_[parms[1]].tableName, handle, configs) !=
          bm::MatchErrorCode::SUCCESS)
        throw P4Exception (P4ErrorCode::NO_SUCCESS);
    }
  else // Indirect meter
    {
      size_t idx = StrToInt (parms[2]);
      if (p4Core_->meter_set_rates (0, parms[1], idx, configs) != 0)
        throw P4Exception (P4ErrorCode::NO_SUCCESS);
    }
}

bm::MatchKeyParam
P4SwitchInterface::ParseMatchField (const std::string &field, bm::MatchKeyParam::Type matchType)
{
  switch (matchType)
    {
    case bm::MatchKeyParam::Type::EXACT:
      return bm::MatchKeyParam (matchType, HexStrToBytes (field));

      case bm::MatchKeyParam::Type::LPM: {
        int pos = field.find ("/");
        std::string prefix = field.substr (0, pos);
        unsigned int length = StrToInt (field.substr (pos + 1));
        return bm::MatchKeyParam (matchType, HexStrToBytes (prefix), int (length));
      }

      case bm::MatchKeyParam::Type::TERNARY: {
        int pos = field.find ("&&&");
        std::string key = HexStrToBytes (field.substr (0, pos));
        std::string mask = HexStrToBytes (field.substr (pos + 3));
        if (key.size () != mask.size ())
          throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR,
                             "Key and mask lengths do not match.");
        return bm::MatchKeyParam (matchType, key, mask);
      }

    case bm::MatchKeyParam::Type::RANGE:
      // TO DO: Add range match type logic here
      throw P4Exception (P4ErrorCode::MATCH_TYPE_ERROR, "Range match type is not implemented.");

    case bm::MatchKeyParam::Type::VALID:
      // TO DO: Add valid match type logic here
      throw P4Exception (P4ErrorCode::MATCH_TYPE_ERROR, "Valid match type is not implemented.");

    default:
      throw P4Exception (P4ErrorCode::MATCH_TYPE_ERROR, "Unknown match type.");
    }
}

bm::Meter::rate_config_t
P4SwitchInterface::ParseRateConfig (const std::string &rateBurstStr)
{
  // `std::string::find` returns size_t type, replace int with size_t
  std::string::size_type pos = rateBurstStr.find (":");
  if (pos == std::string::npos)
    throw P4Exception (P4ErrorCode::PARAMETER_NUM_ERROR, "Rate/burst format is invalid.");

  bm::Meter::rate_config_t rateConfig;

  // Make sure to use the appropriate string parsing functions
  rateConfig.info_rate = StrToDouble (rateBurstStr.substr (0, pos));
  rateConfig.burst_size = StrToInt (rateBurstStr.substr (pos + 1));
  return rateConfig;
}

// void
// P4SwitchInterface::Init()
// {
// 	// Initialize the P4 core model

// 	int status = 0;

// 	//  Several methods of populating flowtable
//     if (populateFLowTableWay_ == LOCAL_CALL)
//     {
//         /**
//          * @brief This mode can only deal with "exact" matching table, the "lpm" matching
//          * and other method can not use. @todo -mingyu
//          */
// 		std::vector<char*> args;
// 		args.push_back(nullptr);
// 		args.push_back(jsonPath_.data());
// 		p4Core_->InitFromCommandLineOptionsLocal(static_cast<int>(args.size()), args.data());

// 		ReadP4Info();
// 		PopulateFlowTable();
// 		// ViewFlowtableEntryNum();
//     }
//     else if (populateFLowTableWay_ == RUNTIME_CLI)
//     {
//         /**
//          * @brief start thrift server , use runtime_CLI populate flowtable
//          * This method is from src
//          * This will connect to the simple_switch thrift server and input the command.
//          * by now the bm::switch and the bm::simple_switch is not the same thing, so
//          *  the "sswitch_runtime::get_handler()" by now can not use. @todo -mingyu
//          */

//         // status = this->init_from_command_line_options(argc, argv, m_argParser);
//         // int thriftPort = this->get_runtime_port();
//         // std::cout << "thrift port : " << thriftPort << std::endl;
//         // bm_runtime::start_server(this, thriftPort);
//         // //@todo BUG: THIS MAY CHANGED THE API
//         // using ::sswitch_runtime::SimpleSwitchIf;
//         // using ::sswitch_runtime::SimpleSwitchProcessor;
//         // bm_runtime::add_service<SimpleSwitchIf, SimpleSwitchProcessor>(
//         //         "simple_switch", sswitch_runtime::get_handler(this));
//     }
//     else if (populateFLowTableWay_ == NS3PIFOTM)
//     {
//         /**
//          * @brief This method for setting the json file and populate the flow table
//          * It is taken from "ns3-PIFO-TM", check in github: https://github.com/PIFO-TM/ns3-bmv2
//          */

//         static int p4_switch_ctrl_plane_thrift_port =
//             9090; // the thrift port will from 9090 increase with 1.

//         bm::OptionsParser opt_parser;
//         opt_parser.config_file_path = P4GlobalVar::g_p4JsonPath;
//         opt_parser.debugger_addr = std::string("ipc:///tmp/bmv2-") +
//                                    std::to_string(p4_switch_ctrl_plane_thrift_port) +
//                                    std::string("-debug.ipc");
//         opt_parser.notifications_addr = std::string("ipc:///tmp/bmv2-") +
//                                         std::to_string(p4_switch_ctrl_plane_thrift_port) +
//                                         std::string("-notifications.ipc");
//         opt_parser.file_logger = std::string("/tmp/bmv2-") +
//                                  std::to_string(p4_switch_ctrl_plane_thrift_port) +
//                                  std::string("-pipeline.log");
//         opt_parser.thrift_port = p4_switch_ctrl_plane_thrift_port++;
//         opt_parser.console_logging = true;

//         //! Initialize the switch using an bm::OptionsParser instance.
//         int status = p4Core_->init_from_options_parser(opt_parser);
//         if (status != 0)
//         {
//             std::exit(status);
//         }

//         int port = p4Core_->get_runtime_port();
//         bm_runtime::start_server(p4Core_, port);

//         // std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string(port)
//         //                  + " < " + P4GlobalVar::g_flowTablePath; // this will have
//         // log file output for tables

//         std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string(port) + " < " +
//                           P4GlobalVar::g_flowTablePath + " > /dev/null 2>&1";
//         int resultsys = std::system(cmd.c_str());
//         if (resultsys != 0)
//         {
//             std::cerr << "Error executing command." << std::endl;
//         }
//         // bm_runtime::stop_server();
//     }

//     if (status != 0)
//     {
//         NS_LOG_LOGIC("ERROR: the P4 Model switch init failed in P4Switch::init.");
//         std::exit(status);
//         return;
//     }
// }

void
P4SwitchInterface::Init ()
{
  NS_LOG_FUNCTION (this); // Log function entry

  int status = 0; // Status flag for initialization

  // Initialize P4 core model based on the flow table population method
  if (populateFLowTableWay_ == LOCAL_CALL)
    {
      /**
         * @brief LOCAL_CALL mode supports only "exact" match tables. 
         * Other match types like "lpm" are not supported in this mode.
         */
      NS_LOG_INFO ("Initializing P4Switch with LOCAL_CALL mode.");

      std::vector<char *> args;
      args.push_back (nullptr);
      args.push_back (jsonPath_.data ());

      // Initialize P4 core with local command line options
      status =
          p4Core_->InitFromCommandLineOptionsLocal (static_cast<int> (args.size ()), args.data ());
      if (status != 0)
        {
          NS_LOG_ERROR ("Failed to initialize P4Switch with local command line options.");
          return; // Avoid exiting simulation
        }

      // Read and populate flow table
      ReadP4Info ();
      PopulateFlowTable ();
    }
  else if (populateFLowTableWay_ == RUNTIME_CLI)
    {
      /**
         * @brief RUNTIME_CLI mode starts a Thrift server and uses runtime_CLI
         * commands to populate the flow table.
         */
      NS_LOG_INFO ("Initializing P4Switch with RUNTIME_CLI mode.");
      // Code for RUNTIME_CLI initialization is currently commented out.
      // Uncomment and adapt as needed based on specific requirements.
      // TODO: Implement RUNTIME_CLI initialization logic
    }
  else if (populateFLowTableWay_ == NS3PIFOTM)
    {
      /**
         * @brief NS3PIFOTM mode initializes the switch using a JSON file
         * and populates the flow table using P4GlobalVar paths.
         */
      NS_LOG_INFO ("Initializing P4Switch with NS3PIFOTM mode.");

      static int p4_switch_ctrl_plane_thrift_port = 9090;

      bm::OptionsParser opt_parser;
      opt_parser.config_file_path = P4GlobalVar::g_p4JsonPath;
      opt_parser.debugger_addr =
          "ipc:///tmp/bmv2-" + std::to_string (p4_switch_ctrl_plane_thrift_port) + "-debug.ipc";
      opt_parser.notifications_addr = "ipc:///tmp/bmv2-" +
                                      std::to_string (p4_switch_ctrl_plane_thrift_port) +
                                      "-notifications.ipc";
      opt_parser.file_logger =
          "/tmp/bmv2-" + std::to_string (p4_switch_ctrl_plane_thrift_port) + "-pipeline.log";
      opt_parser.thrift_port = p4_switch_ctrl_plane_thrift_port++;
      opt_parser.console_logging = false;

      // Initialize the switch
      status = 0;
      status = p4Core_->init_from_options_parser (opt_parser);
      if (status != 0)
        {
          NS_LOG_ERROR ("Failed to initialize P4Switch with NS3PIFOTM mode.");
          return; // Avoid exiting simulation
        }

      // Start the runtime server
      int port = p4Core_->get_runtime_port ();
      bm_runtime::start_server (p4Core_, port);

      // Execute CLI command to populate the flow table
      std::string cmd = "simple_switch_CLI --thrift-port " + std::to_string (port) + " < " +
                        P4GlobalVar::g_flowTablePath + " > /dev/null 2>&1";

      // sleep for 4 second to avoid the server not ready
      sleep (4);

      int result = std::system (cmd.c_str ());
      if (result != 0)
        {
          NS_LOG_ERROR ("Error executing flow table population command: " << cmd);
        }

      // Note: Consider stopping the server if needed
      // bm_runtime::stop_server();
    }
  else
    {
      NS_LOG_WARN ("Unknown populateFlowTableWay_ method specified.");
    }

  if (status != 0)
    {
      NS_LOG_ERROR ("P4Switch initialization failed with status: " << status);
      return;
    }

  NS_LOG_INFO ("P4Switch initialization completed successfully.");
}

} // namespace ns3
