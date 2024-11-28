#include "ns3/log.h"
#include "p4-switch-interface.h"
#include "p4-exception-handle.h"
#include "format-utils.h"

#include <fstream>
#include <unordered_map>
#include <iostream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("P4SwitchInterface");

NS_OBJECT_ENSURE_REGISTERED(P4SwitchInterface);

TypeId P4SwitchInterface::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::P4SwitchInterface")
		.SetParent<Object>()
		.SetGroupName("P4SwitchInterface")
		;
	return tid;
}

P4SwitchInterface::P4SwitchInterface()
{
	NS_LOG_FUNCTION(this);
}

P4SwitchInterface::~P4SwitchInterface()
{
	//********TO DO (Whether need delete p4Core_)*******************
	/*if(p4Core_!=nullptr)
	{
		delete p4Core_;
		p4Core_=nullptr;
	}*/
	//***************************************************************
	NS_LOG_FUNCTION(this);
}

void P4SwitchInterface::PopulateFlowTable()
{
    // Attempt to open the flow table file at the specified path
    std::ifstream fileStream(flowTablePath_);
    if (!fileStream.is_open())
    {
        NS_LOG_ERROR("Unable to open flow table file: " << flowTablePath_);
        return;
    }

    const int maxBufferSize = 500; // Define the maximum buffer size for reading rows
    std::string line;             // String to hold each line from the file

	NS_LOG_INFO("Starting to populate the flow table from: " << flowTablePath_);

    // Read the file line by line
    while (std::getline(fileStream, line))
    {
        // Process each line to populate the flow table
        if (!line.empty())
        {
			NS_LOG_DEBUG("Processing line: " << line);
            ParsePopulateFlowTableCommand(line);
        }
    }

	NS_LOG_INFO("Finished populating the flow table.");
    // Close the file stream (automatically handled by RAII)
    fileStream.close();
}


void P4SwitchInterface::ReadP4Info()
{
    NS_LOG_INFO("Reading P4Info from file: " << p4InfoPath_);

    // Opening a file
    std::ifstream topgen(p4InfoPath_);
    if (!topgen.is_open())
    {
        NS_LOG_ERROR("Failed to open P4Info file: " << p4InfoPath_);
        abort(); // Terminate the program when the file cannot be opened
    }

    std::string line;
    std::istringstream lineBuffer;
    std::string elementType;

    std::string tableName, matchType, meterName, counterName;
    unsigned int isDirect;

    while (std::getline(topgen, line))
    {
        lineBuffer.clear();
        lineBuffer.str(line);

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
                NS_LOG_ERROR("Undefined match type in table: " << matchType);
            }
        }
        else if (elementType == "meter")
        {
            lineBuffer >> meterName >> isDirect >> tableName;
            meters_[meterName].tableName = tableName;
            meters_[meterName].isDirect = (isDirect == 1);

            NS_LOG_DEBUG("Parsed meter: " << meterName 
                          << " (isDirect=" << meters_[meterName].isDirect 
                          << ", tableName=" << tableName << ")");
        }
        else if (elementType == "counter")
        {
            lineBuffer >> counterName >> isDirect >> tableName;
            counters_[counterName].isDirect = (isDirect == 1);
            counters_[counterName].tableName = tableName;

            NS_LOG_DEBUG("Parsed counter: " << counterName 
                          << " (isDirect=" << counters_[counterName].isDirect 
                          << ", tableName=" << tableName << ")");
        }
        else
        {
            NS_LOG_WARN("Undefined element type in P4Info: " << elementType);
        }
    }

    NS_LOG_INFO("Finished reading P4Info from file: " << p4InfoPath_);
}

void P4SwitchInterface::ViewFlowtableEntryNum()
{
    NS_LOG_INFO("Viewing the number of entries for each flow table.");

    // Iterate over the flowTables_ map
    for (auto iter = flowTables_.begin(); iter != flowTables_.end(); ++iter)
    {
        const std::string& tableName = iter->first; // Get the table name
        std::string command = "table_num_entries " + tableName;

        NS_LOG_DEBUG("Sending command to retrieve entry count for table: " << tableName);

        // Execute the command to get table entry information
        ParseAttainFlowTableInfoCommand(command);

        // If needed, replace with actual functionality to retrieve the entry count
        // Example:
        // size_t numEntries = 0;
        // mt_get_num_entries(0, tableName, &numEntries);
        // NS_LOG_INFO("Flow table '" << tableName << "' has " << numEntries << " entries.");
    }

    NS_LOG_INFO("Finished viewing flow table entry numbers.");
}

void P4SwitchInterface::AttainSwitchFlowTableInfo()
{
    NS_LOG_INFO("Starting to attain switch flow table information from file: " << viewFlowTablePath_);

    // Open the file at the specified path
    std::ifstream fileStream(viewFlowTablePath_);
    if (!fileStream.is_open())
    {
        NS_LOG_ERROR("Failed to open file: " << viewFlowTablePath_);
        return;
    }

    std::string line;

    // Read the file line by line
    while (std::getline(fileStream, line))
    {
        if (!line.empty())
        {
            NS_LOG_DEBUG("Processing command: " << line);
            ParseAttainFlowTableInfoCommand(line); // Process the command
        }
    }

    NS_LOG_INFO("Finished processing switch flow table information from file: " << viewFlowTablePath_);
}

void P4SwitchInterface::ParseAttainFlowTableInfoCommand(const std::string& commandRow)
{
	NS_LOG_INFO("Processing command: " << commandRow);

	// Split the command row into parameters
	std::vector<std::string> parms;
	size_t lastPos = 0, currentPos = 0;
	while ((currentPos = commandRow.find(' ', lastPos)) != std::string::npos)
	{
		parms.emplace_back(commandRow.substr(lastPos, currentPos - lastPos));
		lastPos = currentPos + 1;
	}
	if (lastPos < commandRow.size())
	{
		parms.emplace_back(commandRow.substr(lastPos));
	}

	// Ensure we have at least one parameter to process
	if (parms.empty())
	{
		NS_LOG_WARN("Empty command row received.");
		return;
	}
	
	// Retrieve the command type
	unsigned int commandType = 0;
	try
	{
		commandType = SwitchApi::g_apiMap.at(parms[0]);
	}
	catch (const std::out_of_range&)
	{
		NS_LOG_ERROR("Invalid command type: " << parms[0]);
		return;
	}

    try
    {
        // Ensure p4Core_ is initialized
        if (p4Core_ == nullptr)
        {
            throw P4Exception(P4ErrorCode::P4_SWITCH_POINTER_NULL);
        }

        // Process the command based on the command type
        switch (commandType)
        {
			case SwitchApi::ApiType::MT_GET_NUM_ENTRIES:
				HandleTableNumEntries(parms);
				break;

			case SwitchApi::ApiType::MT_CLEAR_ENTRIES:
				HandleTableClear(parms);
				break;

			case SwitchApi::ApiType::METER_GET_RATES:
				HandleMeterGetRates(parms);
				break;

			case SwitchApi::ApiType::READ_COUNTERS:
				HandleCounterRead(parms);
				break;

			case SwitchApi::ApiType::RESET_COUNTERS:
				HandleCounterReset(parms);
				break;

			case SwitchApi::ApiType::REGISTER_READ:
				HandleRegisterRead(parms);
				break;

			case SwitchApi::ApiType::REGISTER_WRITE:
				HandleRegisterWrite(parms);
				break;

			case SwitchApi::ApiType::REGISTER_RESET:
				HandleRegisterReset(parms);
				break;

			case SwitchApi::ApiType::MT_GET_ENTRY:
				HandleTableDumpEntry(parms);
				break;

			case SwitchApi::ApiType::MT_GET_ENTRIES:
				HandleTableDump(parms);
				break;

			default:
				throw P4Exception(P4ErrorCode::COMMAND_ERROR);
        }
    }
    catch (P4Exception& e)
    {
        NS_LOG_ERROR("P4Exception caught: " << e.what());
        e.ShowExceptionEntry(e.info());
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Standard exception caught: " << e.what());
        P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
        fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
    }
    catch (...)
    {
        NS_LOG_ERROR("Unknown exception caught.");
        P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
        fallbackException.ShowExceptionEntry(commandRow);
    }
}

		// if (parms.size() > 0)
		// {
		// 	unsigned int commandType = SwitchApi::g_apiMap[parms[0]];
		// 	try {
		// 		if (p4Core_ == nullptr)
		// 			throw P4Exception(P4ErrorCode::P4_SWITCH_POINTER_NULL);
		// 		switch (commandType)
		// 		{
		// 		case TABLE_NUM_ENTRIES: { //table_num_entries <table name>
		// 			try {
		// 				if (parms.size() == 2)
		// 				{
		// 					size_t num_entries;
		// 					if (p4Core_->mt_get_num_entries(0, parms[1], &num_entries) != bm::MatchErrorCode::SUCCESS)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 					std::cout << parms[1] << " entry num: " << num_entries << std::endl;
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case TABLE_CLEAR: {// table_clear <table name>
		// 			try {
		// 				if (parms.size() == 2)
		// 				{
		// 					if (p4Core_->mt_clear_entries(0, parms[1], false) != bm::MatchErrorCode::SUCCESS)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case METER_GET_RATES: {//meter_get_rates <name> <index>
		// 			try {
		// 				if (parms.size() == 3)
		// 				{
		// 					if (meters_.count(parms[1]) > 0)
		// 					{
		// 						if (meters_[parms[1]].isDirect)//direct
		// 						{
		// 							bm::entry_handle_t handle(StrToInt(parms[2]));
		// 							std::vector<bm::Meter::rate_config_t> configs;
		// 							if (p4Core_->mt_get_meter_rates(0, meters_[parms[1]].tableName, handle, &configs) != bm::MatchErrorCode::SUCCESS)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 							for (size_t i = 0; i < configs.size(); i++)
		// 							{
		// 								std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
		// 							}
		// 						}
		// 						else//indirect
		// 						{
		// 							size_t idx(StrToInt(parms[2]));
		// 							std::vector<bm::Meter::rate_config_t> configs;
		// 							if (p4Core_->meter_get_rates(0, parms[1], idx, &configs) != 0)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 							for (size_t i = 0; i < configs.size(); i++)
		// 							{
		// 								std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
		// 							}
		// 						}
		// 					}
		// 					else
		// 						throw P4Exception(P4ErrorCode::METER_NO_EXIST);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case COUNTER_READ: { //counter_read <name> <index>
		// 			try {
		// 				if (parms.size() == 3)
		// 				{
		// 					if (counters_.count(parms[1]) > 0)
		// 					{
		// 						if (counters_[parms[1]].isDirect)//direct
		// 						{
		// 							bm::entry_handle_t handle(StrToInt(parms[2]));
		// 							bm::MatchTableAbstract::counter_value_t bytes;
		// 							bm::MatchTableAbstract::counter_value_t packets;
		// 							if (p4Core_->mt_read_counters(0, counters_[parms[1]].tableName, handle, &bytes, &packets) != bm::MatchErrorCode::SUCCESS)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 							std::cout << "counter " << parms[1] << "[" << handle << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
		// 						}
		// 						else
		// 						{
		// 							size_t index(StrToInt(parms[2]));
		// 							bm::MatchTableAbstract::counter_value_t bytes;
		// 							bm::MatchTableAbstract::counter_value_t packets;
		// 							if (p4Core_->read_counters(0, parms[1], index, &bytes, &packets) != 0)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 							std::cout << "counter " << parms[1] << "[" << index << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
		// 						}
		// 					}
		// 					else
		// 						throw P4Exception(P4ErrorCode::COUNTER_NO_EXIST);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case COUNTER_RESET: {//counter_reset <name>
		// 			try {
		// 				if (parms.size() == 2)
		// 				{
		// 					if (counters_.count(parms[1]) > 0)
		// 					{
		// 						if (counters_[parms[1]].isDirect)//direct
		// 						{
		// 							if (p4Core_->mt_reset_counters(0, counters_[parms[1]].tableName) != bm::MatchErrorCode::SUCCESS)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 						}
		// 						else //indirect
		// 						{
		// 							if (p4Core_->reset_counters(0, parms[1]) != 0)
		// 								throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 						}
		// 					}
		// 					else
		// 						throw P4Exception(P4ErrorCode::COUNTER_NO_EXIST);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case REGISTER_READ: {//register_read <name> [index]
		// 			try {
		// 				if (parms.size() == 3)
		// 				{
		// 					size_t index(StrToInt(parms[2]));
		// 					bm::Data value;
		// 					if (p4Core_->register_read(0, parms[1], index, &value) != 0)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 					std::cout << "register " << parms[1] << "[" << index << "] value :" << value << std::endl;
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case REGISTER_WRITE: { //register_write <name> <index> <value>
		// 			try {
		// 				if (parms.size() == 4)
		// 				{
		// 					size_t index(StrToInt(parms[2]));
		// 					bm::Data value(parms[3]);
		// 					if (p4Core_->register_write(0, parms[1], index, value) != 0)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case REGISTER_RESET: {//register_reset <name>
		// 			try {
		// 				if (parms.size() == 2)
		// 				{
		// 					if (p4Core_->register_reset(0, parms[1]) != 0)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case TABLE_DUMP_ENTRY: {// table_dump_entry <table name> <entry handle>
		// 			try {
		// 				if (parms.size() == 3)
		// 				{
		// 					bm::entry_handle_t handle(StrToInt(parms[2]));
		// 					bm::MatchTable::Entry entry;
		// 					if (p4Core_->mt_get_entry(0, parms[1], handle, &entry) != bm::MatchErrorCode::SUCCESS)
		// 						throw P4Exception(P4ErrorCode::NO_SUCCESS);
		// 					std::cout << parms[1] << " entry " << handle << " :" << std::endl;
		// 					std::cout << "MatchKey:";
		// 					for (size_t i = 0; i < entry.match_key.size(); i++)
		// 					{
		// 						std::cout << entry.match_key[i].key << " ";
		// 					}
		// 					std::cout << std::endl << "ActionData:";
		// 					for (size_t i = 0; i < entry.action_data.action_data.size(); i++)
		// 					{
		// 						std::cout << entry.action_data.action_data[i] << " ";
		// 					}
		// 					std::cout << std::endl;
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		case TABLE_DUMP: {//table_dump <table name>
		// 			try {
		// 				if (parms.size() == 2)
		// 				{
		// 					std::vector<bm::MatchTable::Entry> entries;
		// 					entries = p4Core_->mt_get_entries(0, parms[1]);
		// 					// TO DO: output entries info
		// 				}
		// 				else
		// 					throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
		// 			}
		// 			catch (P4Exception& e)
		// 			{
		// 				NS_LOG_ERROR("Exception caught: " << e.what());
		// 				e.ShowExceptionEntry(e.info());
		// 			}
		// 			catch (const std::exception& e) {
		// 				NS_LOG_ERROR("Standard exception caught: " << e.what());
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
		// 				fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
		// 			} 
		// 			catch (...) {
		// 				NS_LOG_ERROR("Unknown exception caught.");
		// 				P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
		// 				fallbackException.ShowExceptionEntry(commandRow);
		// 			}
		// 			break;
		// 		}
		// 		default: {
		// 			throw P4Exception(P4ErrorCode::COMMAND_ERROR);
		// 			break;
		// 		}
		// 		}
		// 	}
		// 	catch (P4Exception& e)
		// 	{
		// 		NS_LOG_ERROR("Exception caught: " << e.what());
		// 	}
		// }
	// }


void P4SwitchInterface::HandleTableDumpEntry(const std::vector<std::string>& parms)
{
    if (parms.size() != 3)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for TABLE_DUMP_ENTRY");
    }

    bm::entry_handle_t handle(StrToInt(parms[2])); // Parse the entry handle
    bm::MatchTable::Entry entry;

    // Retrieve the entry
    if (p4Core_->mt_get_entry(0, parms[1], handle, &entry) != bm::MatchErrorCode::SUCCESS)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to retrieve table entry for table: " + parms[1]);
    }

    // Display the entry information
    std::ostringstream oss;
    oss << parms[1] << " entry " << handle << " :\nMatchKey: ";
    for (const auto& key : entry.match_key)
    {
        oss << key.key << " ";
    }
    oss << "\nActionData: ";
    for (const auto& data : entry.action_data.action_data)
    {
        oss << data << " ";
    }

    NS_LOG_INFO(oss.str());
}

void P4SwitchInterface::HandleTableNumEntries(const std::vector<std::string>& parms)
{
    if (parms.size() != 2)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for TABLE_NUM_ENTRIES");
    }

    size_t numEntries = 0;

    // Retrieve the number of entries
    if (p4Core_->mt_get_num_entries(0, parms[1], &numEntries) != bm::MatchErrorCode::SUCCESS)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to retrieve table entries for table: " + parms[1]);
    }

    NS_LOG_INFO("Table " << parms[1] << " has " << numEntries << " entries.");
}

void P4SwitchInterface::HandleTableClear(const std::vector<std::string>& parms)
{
    if (parms.size() != 2)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for TABLE_CLEAR");
    }

    // Clear the table
    if (p4Core_->mt_clear_entries(0, parms[1], false) != bm::MatchErrorCode::SUCCESS)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to clear entries in table: " + parms[1]);
    }

    NS_LOG_INFO("Successfully cleared entries in table: " << parms[1]);
}

void P4SwitchInterface::HandleMeterGetRates(const std::vector<std::string>& parms)
{
    if (parms.size() != 3)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for METER_GET_RATES");
    }

    if (meters_.count(parms[1]) == 0)
    {
        throw P4Exception(P4ErrorCode::METER_NO_EXIST, "Meter does not exist: " + parms[1]);
    }

    const auto& meter = meters_[parms[1]];
    std::vector<bm::Meter::rate_config_t> configs;

    if (meter.isDirect)
    {
        bm::entry_handle_t handle(StrToInt(parms[2]));
        if (p4Core_->mt_get_meter_rates(0, meter.tableName, handle, &configs) != bm::MatchErrorCode::SUCCESS)
        {
            throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to retrieve meter rates for direct meter: " + parms[1]);
        }
    }
    else
    {
        size_t index = StrToInt(parms[2]);
        if (p4Core_->meter_get_rates(0, parms[1], index, &configs) != 0)
        {
            throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to retrieve meter rates for indirect meter: " + parms[1]);
        }
    }

    for (const auto& config : configs)
    {
        NS_LOG_INFO("Meter " << parms[1] << " RateConfig: info_rate=" << config.info_rate
                             << ", burst_size=" << config.burst_size);
    }
}

void P4SwitchInterface::HandleCounterRead(const std::vector<std::string>& parms)
{
    if (parms.size() != 3)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for COUNTER_READ");
    }

    if (counters_.count(parms[1]) == 0)
    {
        throw P4Exception(P4ErrorCode::COUNTER_NO_EXIST, "Counter does not exist: " + parms[1]);
    }

    const auto& counter = counters_[parms[1]];
    bm::MatchTableAbstract::counter_value_t bytes = 0, packets = 0;

    if (counter.isDirect)
    {
        bm::entry_handle_t handle(StrToInt(parms[2]));
        if (p4Core_->mt_read_counters(0, counter.tableName, handle, &bytes, &packets) != bm::MatchErrorCode::SUCCESS)
        {
            throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to read counter for direct counter: " + parms[1]);
        }
    }
    else
    {
        size_t index = StrToInt(parms[2]);
        if (p4Core_->read_counters(0, parms[1], index, &bytes, &packets) != 0)
        {
            throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to read counter for indirect counter: " + parms[1]);
        }
    }

    NS_LOG_INFO("Counter " << parms[1] << " [" << parms[2] << "]: " << bytes << " bytes, " << packets << " packets.");
}

void P4SwitchInterface::HandleRegisterRead(const std::vector<std::string>& parms)
{
    if (parms.size() != 3)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for REGISTER_READ");
    }

    size_t index = StrToInt(parms[2]);
    bm::Data value;

    // Read the register value
    if (p4Core_->register_read(0, parms[1], index, &value) != 0)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to read register: " + parms[1] + " at index " + parms[2]);
    }

    NS_LOG_INFO("Register " << parms[1] << "[" << index << "] value: " << value);
}

void P4SwitchInterface::HandleRegisterWrite(const std::vector<std::string>& parms)
{
    if (parms.size() != 4)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for REGISTER_WRITE");
    }

    size_t index = StrToInt(parms[2]);
    bm::Data value(parms[3]);

    // Write the value to the register
    if (p4Core_->register_write(0, parms[1], index, value) != 0)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to write to register: " + parms[1] +
                                                     " at index " + parms[2] + " with value " + parms[3]);
    }

    NS_LOG_INFO("Successfully wrote value " << value << " to register " << parms[1] << "[" << index << "].");
}

void P4SwitchInterface::HandleRegisterReset(const std::vector<std::string>& parms)
{
    if (parms.size() != 2)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for REGISTER_RESET");
    }

    // Reset the register
    if (p4Core_->register_reset(0, parms[1]) != 0)
    {
        throw P4Exception(P4ErrorCode::NO_SUCCESS, "Failed to reset register: " + parms[1]);
    }

    NS_LOG_INFO("Successfully reset register: " << parms[1]);
}

void P4SwitchInterface::HandleTableDump(const std::vector<std::string>& parms)
{
    if (parms.size() != 2)
    {
        throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR, "Invalid number of parameters for TABLE_DUMP");
    }

    std::vector<bm::MatchTable::Entry> entries;

    // Retrieve all entries from the table
    entries = p4Core_->mt_get_entries(0, parms[1]);

    if (entries.empty())
    {
        NS_LOG_INFO("Table " << parms[1] << " has no entries.");
        return;
    }

    // Log each entry's details
    std::ostringstream oss;
    oss << "Dumping entries for table: " << parms[1] << "\n";
    for (const auto& entry : entries)
    {
        oss << "Entry:\n  MatchKey: ";
        for (const auto& key : entry.match_key)
        {
            oss << key.key << " ";
        }
        oss << "\n  ActionData: ";
        for (const auto& data : entry.action_data.action_data)
        {
            oss << data << " ";
        }
        oss << "\n";
    }

    NS_LOG_INFO(oss.str());
}


	void P4SwitchInterface::ParsePopulateFlowTableCommand(const std::string& commandRow)
	{
		std::vector<std::string> parms;
		// lastP : the position of the last space in the string "commandRow"
		// curP ： the current position.
		int lastP = 0, curP = 0; 
		for (size_t i = 0; i < commandRow.size(); i++, curP++)
		{
			if (commandRow[i] == ' ')
			{
				parms.push_back(commandRow.substr(lastP, curP - lastP));
				lastP = i + 1;
			}
		}
		if (lastP < curP)
		{
			parms.push_back(commandRow.substr(lastP, curP - lastP));
		}
		if (parms.size() > 0)
		{
			unsigned int commandType = SwitchApi::g_apiMap[parms[0]];
			try {
				if (p4Core_ == nullptr)
					throw P4Exception(P4ErrorCode::P4_SWITCH_POINTER_NULL);
				switch (commandType)
				{
				case TABLE_SET_DEFAULT: {//table_set_default <table name> <action name> <action parameters>
					try {
						//NS_LOG_LOGIC("TABLE_SET_DEFAULT:"<<commandRow);
						if (parms.size() >= 3) {
							bm::ActionData actionData;
							if (parms.size() > 3)// means have ActionData
							{
								for (size_t i = 3; i < parms.size(); i++)
									actionData.push_back_action_data(bm::Data(parms[i]));
							}
							if (p4Core_->mt_set_default_action(0, parms[1], parms[2], actionData) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				// Just support hexadecimal match fields, action parameters

				case TABLE_ADD: {//table_add <table name> <action name> <match fields> => <action parameters> [priority]
					try {
						//NS_LOG_LOGIC("TABLE_ADD:"<<commandRow);
						std::vector<bm::MatchKeyParam> matchKey;
						bm::ActionData actionData;
						bm::entry_handle_t handle;
						bm::MatchKeyParam::Type matchType = flowTables_[parms[1]].matchType;
						unsigned int keyNum = 0;
						unsigned int actionDataNum = 0;
						size_t i;
						for (i = 3; i < parms.size(); i++)
						{
							if (parms[i].compare("=>") != 0)
							{
								keyNum++;
								switch (matchType)
								{
								case bm::MatchKeyParam::Type::EXACT:
								{
									matchKey.push_back(bm::MatchKeyParam(matchType, HexstrToBytes(parms[i])));
									break;
								}
								case bm::MatchKeyParam::Type::LPM:
								{
									// WARNING: interface for LPM, by now not support all use cases.
									int pos = parms[i].find("/");
									std::string prefix = parms[i].substr(0, pos);
									std::string suffix = parms[i].substr(pos + 1);
									unsigned int Length = StrToInt(suffix);

									// int temp_bw_codel_drop_cnt8 = 32;
									// matchKey.push_back(bm::MatchKeyParam(matchType, HexstrToBytes(parms[i], Length), int(Length))); //src
									std::string key = ParseParam(prefix, Length);
									// std::string key = key_pre + "/" + suffix;
									matchKey.push_back(bm::MatchKeyParam(matchType, key, int(Length)));
									break;
								}
								case bm::MatchKeyParam::Type::TERNARY:
								{
									int pos = parms[i].find("&&&");
									std::string key = HexstrToBytes(parms[i].substr(0, pos));
									std::string mask = HexstrToBytes(parms[i].substr(pos + 3));
									if (key.size() != mask.size())
									{
										std::cerr << "key and mask length unmatch!" << std::endl;
									}
									else
									{
										matchKey.push_back(bm::MatchKeyParam(matchType, key, mask));
									}
									break;
								}
								case bm::MatchKeyParam::Type::RANGE:
								{
									// TO DO: handle range match type
									break;
								}
								case bm::MatchKeyParam::Type::VALID:
								{
									// TO DO: handle valid match type
									break;
								}
								default:
								{
									throw P4Exception(P4ErrorCode::MATCH_TYPE_ERROR);
									break;
								}
								}
							}
							else
								break;
						}
						i++;
						int priority;
						//TO DO:judge key_num equal table need key num
						
						if (matchType == bm::MatchKeyParam::Type::EXACT) {
							//NS_LOG_LOGIC("Parse ActionData from index:"<<i);
							for (; i < parms.size(); i++)
							{	
								// Get the action data from parms, right!
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							priority = 0;
							//TO DO:judge action_data_num equal action need num
							if (p4Core_->mt_add_entry(0, parms[1], matchKey, parms[2], actionData, &handle, priority) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else if (matchType == bm::MatchKeyParam::Type::LPM) {
							for (; i < parms.size(); i++)
							{	
								// Get the action data from parms, right!
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							//TO DO:judge action_data_num equal action need num
							if (p4Core_->mt_add_entry(0, parms[1], matchKey, parms[2], actionData, &handle) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else 
						{	
							// ==== TERNARY and Range ====
							// @todo should not use this part, without test.
							for (; i < parms.size() - 1; i++)
							{
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							//TO DO:judge action_data_num equal action need num
							priority = StrToInt(parms[parms.size() - 1]);
							if (p4Core_->mt_add_entry(0, parms[1], matchKey, parms[2], actionData, &handle, priority) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_SET_TIMEOUT: { //table_set_timeout <table_name> <entry handle> <timeout (ms)>
					try {
						if (parms.size() == 4)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							unsigned int ttl_ms(StrToInt(parms[3]));
							if (p4Core_->mt_set_entry_ttl(0, parms[1], handle, ttl_ms) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_MODIFY: {//table_modify <table name> <action name> <entry handle> [action parameters]
					try {
						if (parms.size() >= 4) {
							bm::ActionData actionData;
							bm::entry_handle_t handle(StrToInt(parms[3]));
							unsigned int actionDataNum = 0;
							for (size_t i = 4; i < parms.size(); i++)
							{
								actionDataNum++;
								actionData.push_back_action_data(bm::Data(parms[i]));
							}
							//TO DO:judge action_data_num equal action need num
							if (p4Core_->mt_modify_entry(0, parms[1], handle, parms[2], actionData) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				case TABLE_DELETE: {// table_delete <table name> <entry handle>
					try {
						if (parms.size() == 3)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							if (p4Core_->mt_delete_entry(0, parms[1], handle) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);

					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				case METER_ARRAY_SET_RATES: {//meter_array_set_rates <name> <rate_1>:<burst_1> <rate_2>:<burst_2> ...
					try {
						if (parms.size() > 2) {
							std::vector<bm::Meter::rate_config_t> configs;
							for (size_t i = 2; i < parms.size(); i++)
							{
								int pos = parms[i].find(":");// may be ':' better, should think more...
								std::string rate = parms[i].substr(0, pos);
								std::string burst = parms[i].substr(pos + 1);
								bm::Meter::rate_config_t rateConfig;
								rateConfig.info_rate = StrToDouble(rate);
								rateConfig.burst_size = StrToInt(burst);
								configs.push_back(rateConfig);
							}
							if (p4Core_->meter_array_set_rates(0, parms[1], configs) != 0)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				case METER_SET_RATES: {// meter_set_rates <name> <index> <rate_1>:<burst_1> <rate_2>:<burst_2> ...
					try {
						if (parms.size() > 3) {
							std::vector<bm::Meter::rate_config_t> configs;
							for (size_t i = 3; i < parms.size(); i++)
							{
								int pos = parms[i].find(":");
								std::string rate = parms[i].substr(0, pos);
								std::string burst = parms[i].substr(pos + 1);
								bm::Meter::rate_config_t rateConfig;
								rateConfig.info_rate = StrToDouble(rate);
								rateConfig.burst_size = StrToInt(burst);
								configs.push_back(rateConfig);
							}
							if (meters_[parms[1]].isDirect)//direct
							{
								bm::entry_handle_t handle(StrToInt(parms[2]));
								if (p4Core_->mt_set_meter_rates(0, meters_[parms[1]].tableName, handle, configs) != bm::MatchErrorCode::SUCCESS)
									throw P4Exception(P4ErrorCode::NO_SUCCESS);
							}
							else//indirect
							{
								size_t idx(StrToInt(parms[2]));
								if (p4Core_->meter_set_rates(0, parms[1], idx, configs) != 0)
									throw P4Exception(P4ErrorCode::NO_SUCCESS);
							}
						}
						else
							throw P4Exception(P4ErrorCode::PARAMETER_NUM_ERROR);
					}
					catch (P4Exception& e)
					{
						NS_LOG_ERROR("Exception caught: " << e.what());
						e.ShowExceptionEntry(e.info());
					}
					catch (const std::exception& e) {
						NS_LOG_ERROR("Standard exception caught: " << e.what());
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unexpected std::exception");
						fallbackException.ShowExceptionEntry("Standard exception: " + std::string(e.what()));
					} 
					catch (...) {
						NS_LOG_ERROR("Unknown exception caught.");
						P4Exception fallbackException(P4ErrorCode::OTHER_ERROR, "Unknown exception");
						fallbackException.ShowExceptionEntry(commandRow);
					}
					break;
				}
				default:
				{
					throw P4Exception(P4ErrorCode::COMMAND_ERROR);
					break;
				}
				}
			}
			catch (P4Exception&e) {
				NS_LOG_ERROR("Exception caught: " << e.what());
			}
		}
	}

	void P4SwitchInterface::Init()
	{
		ReadP4Info();
		PopulateFlowTable();
		//ViewFlowtableEntryNum();
	}
}

