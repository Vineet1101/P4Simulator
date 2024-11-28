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
		std::fstream fp;
		fp.open(flowTablePath_);
		if (!fp)
		{
			std::cout << "in P4Model::PopulateFlowTable, " << flowTablePath_ << " can't open." << std::endl;
		}
		else
		{
			const int maxSize = 500;
			char row[maxSize];
			while (fp.getline(row, maxSize))
			{
				ParsePopulateFlowTableCommand(std::string(row));
				//ParseFlowtableCommand(std::string(row));
			}
		}
	}

	void P4SwitchInterface::ReadP4Info()
	{
		std::ifstream topgen;
		topgen.open(p4InfoPath_);

		if (!topgen.is_open())
		{
			std::cout << p4InfoPath_ << " can not open!" << std::endl;
			abort();
		}

		std::istringstream lineBuffer;
		std::string line;

		std::string elementType;
		std::string tableName;
		std::string matchType;
		std::string meterName;
		unsigned int isDirect;
		std::string counterName;

		while (getline(topgen, line))
                {
                        lineBuffer.clear();
                        lineBuffer.str(line);

                        lineBuffer >> elementType;
                        if(elementType.compare("table")==0)
                        {
                                lineBuffer>>tableName>>matchType;
                                if (matchType.compare("exact") == 0)
                                {
                                        flowTables_[tableName].matchType = bm::MatchKeyParam::Type::EXACT;
                                }
                                else
                                {
                                        if (matchType.compare("lpm") == 0)
                                        {
                                                flowTables_[tableName].matchType = bm::MatchKeyParam::Type::LPM;
                                        }
                                        else
                                        {
                                                if (matchType.compare("ternary") == 0)
                                                {
                                                        flowTables_[tableName].matchType = bm::MatchKeyParam::Type::TERNARY;
                                                }
                                                else
                                                {
                                                        if (matchType.compare("valid") == 0)
                                                        {
                                                                flowTables_[tableName].matchType = bm::MatchKeyParam::Type::VALID;
                                                        }
                                                        else
                                                        {
                                                                if (matchType.compare("range") == 0)
                                                                {
                                                                        flowTables_[tableName].matchType = bm::MatchKeyParam::Type::RANGE;
                                                                }
                                                                else
                                                                {
                                                                        std::cerr<< "p4-switch-interface.cc ReadP4Info() MatchType undefined!!!" << std::endl;
                                                                }
                                                        }
                                                }
                                        }
                                }
                                continue;
                        }
                        if(elementType.compare("meter")==0)
                        {
                                lineBuffer>>meterName>>isDirect>>tableName;
								meters_[meterName].tableName=tableName;
                                if(isDirect==1)
                                        meters_[meterName].isDirect=true;
                                else
                                        meters_[meterName].isDirect=false;
                                continue;
                        }
                        if(elementType.compare("counter")==0)
                        {
                                lineBuffer>>counterName>>isDirect>>tableName;
                                if(isDirect==1)
                                {
                                        counters_[counterName].isDirect=true;
                                        counters_[counterName].tableName=tableName;
                                }
                                else
                                {
                                        counters_[counterName].isDirect=false;
                                        counters_[counterName].tableName=tableName;
                                }
                                
                                continue;
                        }
                        std::cerr<<"p4-switch-interface.cc ReadP4Info() ElementType undefined!!!"<<std::endl;
                }



	}

	void P4SwitchInterface::ViewFlowtableEntryNum()
	{
		//table_num_entries <table name>
		typedef std::unordered_map<std::string, FlowTable>::iterator FlowTableIter_t;
		for (FlowTableIter_t iter = flowTables_.begin(); iter != flowTables_.end(); ++iter)
		{
			std::string parm("table_num_entries ");
			ParseAttainFlowTableInfoCommand(parm + iter->first);

			//size_t num_entries;
			//mt_get_num_entries(0, iter->first, &num_entries);
			//std::cout << iter->first << " entry num: " << num_entries << std::endl;
		}
	}

	void P4SwitchInterface::AttainSwitchFlowTableInfo()
	{
		std::fstream fp;
		fp.open(viewFlowTablePath_);
		if (!fp)
		{
			std::cout << "AttainSwitchFlowTableInfo, " << viewFlowTablePath_ << " can't open." << std::endl;
		}
		else
		{
			const int maxSize = 500;
			char row[maxSize];
			while (fp.getline(row, maxSize))
			{
				ParseAttainFlowTableInfoCommand(std::string(row));
			}
		}
	}

	void P4SwitchInterface::ParseAttainFlowTableInfoCommand(const std::string& commandRow)
	{
		std::vector<std::string> parms;
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
				case TABLE_NUM_ENTRIES: { //table_num_entries <table name>
					try {
						if (parms.size() == 2)
						{
							size_t num_entries;
							if (p4Core_->mt_get_num_entries(0, parms[1], &num_entries) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
							std::cout << parms[1] << " entry num: " << num_entries << std::endl;
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
				case TABLE_CLEAR: {// table_clear <table name>
					try {
						if (parms.size() == 2)
						{
							if (p4Core_->mt_clear_entries(0, parms[1], false) != bm::MatchErrorCode::SUCCESS)
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
				case METER_GET_RATES: {//meter_get_rates <name> <index>
					try {
						if (parms.size() == 3)
						{
							if (meters_.count(parms[1]) > 0)
							{
								if (meters_[parms[1]].isDirect)//direct
								{
									bm::entry_handle_t handle(StrToInt(parms[2]));
									std::vector<bm::Meter::rate_config_t> configs;
									if (p4Core_->mt_get_meter_rates(0, meters_[parms[1]].tableName, handle, &configs) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
									for (size_t i = 0; i < configs.size(); i++)
									{
										std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
									}
								}
								else//indirect
								{
									size_t idx(StrToInt(parms[2]));
									std::vector<bm::Meter::rate_config_t> configs;
									if (p4Core_->meter_get_rates(0, parms[1], idx, &configs) != 0)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
									for (size_t i = 0; i < configs.size(); i++)
									{
										std::cout << "info_rate:" << configs[i].info_rate << " burst_size:" << configs[i].burst_size << std::endl;
									}
								}
							}
							else
								throw P4Exception(P4ErrorCode::METER_NO_EXIST);
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
				case COUNTER_READ: { //counter_read <name> <index>
					try {
						if (parms.size() == 3)
						{
							if (counters_.count(parms[1]) > 0)
							{
								if (counters_[parms[1]].isDirect)//direct
								{
									bm::entry_handle_t handle(StrToInt(parms[2]));
									bm::MatchTableAbstract::counter_value_t bytes;
									bm::MatchTableAbstract::counter_value_t packets;
									if (p4Core_->mt_read_counters(0, counters_[parms[1]].tableName, handle, &bytes, &packets) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
									std::cout << "counter " << parms[1] << "[" << handle << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
								}
								else
								{
									size_t index(StrToInt(parms[2]));
									bm::MatchTableAbstract::counter_value_t bytes;
									bm::MatchTableAbstract::counter_value_t packets;
									if (p4Core_->read_counters(0, parms[1], index, &bytes, &packets) != 0)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
									std::cout << "counter " << parms[1] << "[" << index << "] size:" << bytes << " bytes " << packets << " packets" << std::endl;
								}
							}
							else
								throw P4Exception(P4ErrorCode::COUNTER_NO_EXIST);
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
				case COUNTER_RESET: {//counter_reset <name>
					try {
						if (parms.size() == 2)
						{
							if (counters_.count(parms[1]) > 0)
							{
								if (counters_[parms[1]].isDirect)//direct
								{
									if (p4Core_->mt_reset_counters(0, counters_[parms[1]].tableName) != bm::MatchErrorCode::SUCCESS)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
								}
								else //indirect
								{
									if (p4Core_->reset_counters(0, parms[1]) != 0)
										throw P4Exception(P4ErrorCode::NO_SUCCESS);
								}
							}
							else
								throw P4Exception(P4ErrorCode::COUNTER_NO_EXIST);
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
				case REGISTER_READ: {//register_read <name> [index]
					try {
						if (parms.size() == 3)
						{
							size_t index(StrToInt(parms[2]));
							bm::Data value;
							if (p4Core_->register_read(0, parms[1], index, &value) != 0)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
							std::cout << "register " << parms[1] << "[" << index << "] value :" << value << std::endl;
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
				case REGISTER_WRITE: { //register_write <name> <index> <value>
					try {
						if (parms.size() == 4)
						{
							size_t index(StrToInt(parms[2]));
							bm::Data value(parms[3]);
							if (p4Core_->register_write(0, parms[1], index, value) != 0)
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
				case REGISTER_RESET: {//register_reset <name>
					try {
						if (parms.size() == 2)
						{
							if (p4Core_->register_reset(0, parms[1]) != 0)
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
				case TABLE_DUMP_ENTRY: {// table_dump_entry <table name> <entry handle>
					try {
						if (parms.size() == 3)
						{
							bm::entry_handle_t handle(StrToInt(parms[2]));
							bm::MatchTable::Entry entry;
							if (p4Core_->mt_get_entry(0, parms[1], handle, &entry) != bm::MatchErrorCode::SUCCESS)
								throw P4Exception(P4ErrorCode::NO_SUCCESS);
							std::cout << parms[1] << " entry " << handle << " :" << std::endl;
							std::cout << "MatchKey:";
							for (size_t i = 0; i < entry.match_key.size(); i++)
							{
								std::cout << entry.match_key[i].key << " ";
							}
							std::cout << std::endl << "ActionData:";
							for (size_t i = 0; i < entry.action_data.action_data.size(); i++)
							{
								std::cout << entry.action_data.action_data[i] << " ";
							}
							std::cout << std::endl;
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
				case TABLE_DUMP: {//table_dump <table name>
					try {
						if (parms.size() == 2)
						{
							std::vector<bm::MatchTable::Entry> entries;
							entries = p4Core_->mt_get_entries(0, parms[1]);
							// TO DO: output entries info
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
				default: {
					throw P4Exception(P4ErrorCode::COMMAND_ERROR);
					break;
				}
				}
			}
			catch (P4Exception& e)
			{
				NS_LOG_ERROR("Exception caught: " << e.what());
			}
		}
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

