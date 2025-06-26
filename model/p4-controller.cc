#include "ns3/p4-controller.h"

#include "ns3/log.h"

#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("P4Controller");
NS_OBJECT_ENSURE_REGISTERED(P4Controller);

TypeId P4Controller::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::P4Controller")
                          .SetParent<Object>()
                          .SetGroupName("P4Controller");
  return tid;
}

P4Controller::P4Controller() { NS_LOG_FUNCTION(this); }

P4Controller::~P4Controller() { NS_LOG_FUNCTION(this); }

void P4Controller::RegisterSwitch(ns3::Ptr<ns3::P4SwitchNetDevice> sw) {
  m_connectedSwitches.push_back(sw);
  NS_LOG_INFO("Switch registered successfully");
}

uint32_t P4Controller::GetN() { return m_connectedSwitches.size(); }

void P4Controller::ViewAllSwitchFlowTableInfo() {
  NS_LOG_INFO("\n==== Viewing All P4 Switch Flow Tables ====\n");
  for (uint32_t i = 0; i < m_connectedSwitches.size(); ++i) {
    ViewP4SwitchFlowTableInfo(i);
  }
  NS_LOG_INFO("==========================================\n");
}

void P4Controller::ViewP4SwitchFlowTableInfo(uint32_t index) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_ERROR("[ERROR] Invalid switch index: " << index << "\n");
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  std::string path = sw->GetFlowTablePath();
  std::ifstream file(path);

  if (!file.is_open()) {
    NS_LOG_ERROR("[ERROR] Could not open flow table file at: " << path << "\n");
    return;
  }

  NS_LOG_INFO("Flow Table for Switch " << index << "\n");
  std::string line;
  while (std::getline(file, line)) {
    NS_LOG_INFO("  " << line << "\n");
  }
  file.close();
}

void P4Controller::PrintTableEntryCount(uint32_t index,
                                        const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_WARN("Switch " << index << " has no v1model core (core is null)");
    return;
  }

  int num = core->GetNumEntries(tableName);
  NS_LOG_INFO("Switch " << index << " - Table [" << tableName << "] has " << num
                        << " entries.");
}

void P4Controller::ClearFlowTableEntries(uint32_t index,
                                         const std::string &tableName,
                                         bool resetDefault) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_WARN("Switch " << index << " has no v1model core (core is null)");
    return;
  }
  int rc = core->ClearFlowTableEntries(tableName, resetDefault);

  if (rc == 0) {
    NS_LOG_INFO("Successfully cleared entries in table ["
                << tableName << "] on switch " << index);
  } else {
    NS_LOG_ERROR("Failed to clear entries in table ["
                 << tableName << "] on switch " << index);
  }
}

void P4Controller::AddFlowEntry(uint32_t index, const std::string &tableName,
                                const std::vector<bm::MatchKeyParam> &matchKey,
                                const std::string &actionName,
                                bm::ActionData &&actionData, int priority) {
  NS_LOG_FUNCTION(this << index << tableName << actionName << priority);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  bm::entry_handle_t handle;
  int result = core->AddFlowEntry(tableName, matchKey, actionName,
                                  std::move(actionData), &handle, priority);

  if (result == 0) {
    NS_LOG_INFO("Successfully added flow entry to table ["
                << tableName << "] on switch " << index
                << " (handle = " << handle << ")");
  } else {
    NS_LOG_ERROR("Failed to add flow entry to table ["
                 << tableName << "] on switch " << index
                 << ", result code = " << result);
  }
}

void P4Controller::SetDefaultAction(uint32_t index,
                                    const std::string &tableName,
                                    const std::string &actionName,
                                    bm::ActionData &&actionData) {
  NS_LOG_FUNCTION(this << index << tableName << actionName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status =
      core->SetDefaultAction(tableName, actionName, std::move(actionData));

  if (status == 0) {
    NS_LOG_INFO("Successfully set default action ["
                << actionName << "] for table [" << tableName << "] on switch "
                << index);
  } else {
    NS_LOG_ERROR("Failed to set default action for table ["
                 << tableName << "] on switch " << index
                 << ", code = " << status);
  }
}

void P4Controller::ResetDefaultEntry(uint32_t index,
                                     const std::string &tableName) {
  NS_LOG_FUNCTION(this << index << tableName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ResetDefaultEntry(tableName);

  if (status == 0) {
    NS_LOG_INFO("Successfully reset default entry for table ["
                << tableName << "] on switch " << index);
  } else {
    NS_LOG_ERROR("Failed to reset default entry for table ["
                 << tableName << "] on switch " << index
                 << ", code = " << status);
  }
}

void P4Controller::DeleteFlowEntry(uint32_t index, const std::string &tableName,
                                   bm::entry_handle_t handle) {
  NS_LOG_FUNCTION(this << index << tableName << handle);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->DeleteFlowEntry(tableName, handle);

  if (status == 0) {
    NS_LOG_INFO("Successfully deleted entry (handle = "
                << handle << ") from table [" << tableName << "] on switch "
                << index);
  } else {
    NS_LOG_ERROR("Failed to delete entry from table ["
                 << tableName << "] on switch " << index
                 << ", code = " << status);
  }
}

void P4Controller::ModifyFlowEntry(uint32_t index, const std::string &tableName,
                                   bm::entry_handle_t handle,
                                   const std::string &actionName,
                                   bm::ActionData actionData) {
  NS_LOG_FUNCTION(this << index << tableName << handle << actionName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ModifyFlowEntry(tableName, handle, actionName,
                                     std::move(actionData));

  if (status == 0) {
    NS_LOG_INFO("Modified flow entry for handle " << handle << " in table ["
                                                  << tableName << "] on switch "
                                                  << index);
  } else {
    NS_LOG_ERROR("Failed to modify flow entry for handle "
                 << handle << " in table [" << tableName << "] on switch "
                 << index);
  }
}

void P4Controller::SetEntryTtl(uint32_t index, const std::string &tableName,
                               bm::entry_handle_t handle, unsigned int ttlMs) {
  NS_LOG_FUNCTION(this << index << tableName << handle << ttlMs);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->SetEntryTtl(tableName, handle, ttlMs);

  if (status == 0) {
    NS_LOG_INFO("Set TTL = " << ttlMs << "ms for entry handle " << handle
                             << " in table [" << tableName << "] on switch "
                             << index);
  } else {
    NS_LOG_ERROR("Failed to set TTL for entry handle "
                 << handle << " in table [" << tableName << "] on switch "
                 << index);
  }
}

void P4Controller::PrintFlowEntries(uint32_t index,
                                    const std::string &tableName) {
  NS_LOG_FUNCTION(this << index << tableName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  std::vector<bm::MatchTable::Entry> entries = core->GetFlowEntries(tableName);

  NS_LOG_INFO("Flow entries in table [" << tableName << "] on switch " << index
                                        << ":");

  for (const auto &entry : entries) {
    std::ostringstream oss;
    oss << "  Handle: " << entry.handle << ", Priority: " << entry.priority
        << ", Action: " << entry.action_fn->get_name();
    NS_LOG_INFO(oss.str());
  }

  if (entries.empty()) {
    NS_LOG_INFO("  [No entries found]");
  }
}

void P4Controller::PrintIndirectFlowEntries(uint32_t index,
                                            const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  std::vector<bm::MatchTableIndirect::Entry> entries =
      core->GetIndirectFlowEntries(tableName);
  NS_LOG_INFO("Indirect entries in [" << tableName << "] on switch " << index
                                      << ":");
  NS_LOG_INFO("The size of table is" << entries.size());

  for (const auto &e : entries) {
    NS_LOG_INFO("  Handle: " << e.handle << ", Mbr Handle: " << e.mbr
                             << ", Priority: " << e.priority);
  }
}

void P4Controller::PrintIndirectWsFlowEntries(uint32_t index,
                                              const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  std::vector<bm::MatchTableIndirectWS::Entry> entries =
      core->GetIndirectWsFlowEntries(tableName);
  NS_LOG_INFO("WS Indirect entries in [" << tableName << "] on switch " << index
                                         << ":");
  NS_LOG_INFO("The size of table is" << entries.size());
  for (const auto &e : entries) {
    NS_LOG_INFO("The control flow is reaching here");
    NS_LOG_INFO("  Handle: " << e.handle << ", Group Handle: " << e.grp
                             << ", Priority: " << e.priority);
  }
}

void P4Controller::PrintEntry(uint32_t index, const std::string &tableName,
                              bm::entry_handle_t handle) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTable::Entry entry;
  int rc = core->GetEntry(tableName, handle, &entry);
  if (rc == 0) {
    std::string actionName = entry.action_fn->get_name();
    NS_LOG_INFO("Entry for handle " << handle << ": action = " << actionName
                                    << ", priority = " << entry.priority);

    // Print action parameters
    //     const auto& actionData = entry.action_data;
    //    NS_LOG_INFO(actionData);
  } else {
    NS_LOG_WARN("Failed to get entry for handle " << handle << " from table "
                                                  << tableName);
  }
}

void P4Controller::PrintDefaultEntry(uint32_t index,
                                     const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTable::Entry entry;
  int rc = core->GetDefaultEntry(tableName, &entry);
  if (rc != 0) {
    NS_LOG_WARN("Failed to get default entry from table: " << tableName);
    return;
  }

  std::string actionName = entry.action_fn->get_name();
  NS_LOG_INFO("Default entry for table " << tableName
                                         << ": action = " << actionName);

  // const auto& actionData = entry.action_data.Data;
  // for (size_t i = 0; i < actionData.size(); ++i)
  // {
  //     NS_LOG_INFO("  Param[" << i << "]: " << actionData);
  // }
}

void P4Controller::PrintIndirectDefaultEntry(uint32_t index,
                                             const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTableIndirect::Entry entry;
  int rc = core->GetIndirectDefaultEntry(tableName, &entry);
  if (rc != 0) {
    NS_LOG_WARN(
        "Failed to get indirect default entry for table: " << tableName);
    return;
  }

  NS_LOG_INFO("Indirect default entry for table "
              << tableName << ": mbr_handle = " << entry.mbr);
}

void P4Controller::PrintIndirectWsDefaultEntry(uint32_t index,
                                               const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTableIndirectWS::Entry entry;
  int rc = core->GetIndirectWsDefaultEntry(tableName, &entry);
  if (rc != 0) {
    NS_LOG_WARN(
        "Failed to get indirect WS default entry for table: " << tableName);
    return;
  }

  NS_LOG_INFO("Indirect WS default entry for table "
              << tableName << ": grp_handle = " << entry.grp);
}

void P4Controller::PrintEntryFromKey(
    uint32_t index, const std::string &tableName,
    const std::vector<bm::MatchKeyParam> &matchKey) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTable::Entry entry;
  int rc = core->GetEntryFromKey(tableName, matchKey, &entry);
  if (rc != 0) {
    NS_LOG_WARN("No entry found for key in table " << tableName);
    return;
  }

  NS_LOG_INFO("Entry from key in "
              << tableName << ": action = " << entry.action_fn->get_name()
              << ", priority = " << entry.priority);
}

void P4Controller::PrintIndirectEntryFromKey(
    uint32_t index, const std::string &tableName,
    const std::vector<bm::MatchKeyParam> &matchKey) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTableIndirect::Entry entry;
  int rc = core->GetIndirectEntryFromKey(tableName, matchKey, &entry);
  if (rc != 0) {
    NS_LOG_WARN("Indirect entry not found for key in table " << tableName);
    return;
  }

  NS_LOG_INFO("Indirect Entry from key in ["
              << tableName << "] on switch " << index << ": handle = "
              << entry.handle << ", member handle = " << entry.mbr
              << ", priority = " << entry.priority);
}

void P4Controller::PrintIndirectWsEntryFromKey(
    uint32_t index, const std::string &tableName,
    const std::vector<bm::MatchKeyParam> &matchKey) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("No V1Model core for switch " << index);
    return;
  }

  bm::MatchTableIndirectWS::Entry entry;
  int rc = core->GetIndirectWsEntryFromKey(tableName, matchKey, &entry);
  if (rc != 0) {
    NS_LOG_WARN("Indirect WS entry not found for key in table " << tableName);
    return;
  }

  NS_LOG_INFO("Indirect WS Entry from key in ["
              << tableName << "] on switch " << index << ": handle = "
              << entry.handle << ", member handle = " << entry.mbr
              << ", group handle = " << entry.grp
              << ", priority = " << entry.priority);
}

void P4Controller::ReadCounters(uint32_t index, const std::string &tableName,
                                bm::entry_handle_t handle) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_WARN("No V1Model core found for switch " << index);
    return;
  }

  uint64_t bytes = 0, packets = 0;
  if (core->ReadTableCounters(tableName, handle, &bytes, &packets) == 0) {
    NS_LOG_INFO("Switch " << index << ", Table " << tableName << ", Handle "
                          << handle << " → Packets: " << packets
                          << ", Bytes: " << bytes);
  }
}

void P4Controller::ResetCounters(uint32_t index, const std::string &tableName) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_WARN("No V1Model core found for switch " << index);
    return;
  }

  core->ResetTableCounters(tableName);
}

void P4Controller::WriteCounters(uint32_t index, const std::string &tableName,
                                 bm::entry_handle_t handle, uint64_t bytes,
                                 uint64_t packets) {
  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_WARN("No V1Model core found for switch " << index);
    return;
  }

  core->WriteTableCounters(tableName, handle, bytes, packets);
}

// void
// P4Controller::ReadCounter(uint32_t index, const std::string& counterName,
// size_t counterIndex)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     bm::MatchTableAbstract::counter_value_t bytes, packets;
//     auto rc = core->ReadCounter(counterName, counterIndex, &bytes, &packets);
//     if (rc == bm::Counter::CounterErrorCode::SUCCESS)
//     {
//         NS_LOG_INFO("ReadCounter for " << counterName << "[" << counterIndex
//         << "]: " << bytes
//                                        << " bytes, " << packets << "
//                                        packets");
//     }
// }

// void
// P4Controller::ResetCounter(uint32_t index, const std::string& counterName)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     core->ResetCounter(counterName);
// }

// void
// P4Controller::WriteCounter(uint32_t index,
//                            const std::string& counterName,
//                            size_t counterIndex,
//                            bm::MatchTableAbstract::counter_value_t bytes,
//                            bm::MatchTableAbstract::counter_value_t packets)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     core->WriteCounter(counterName, counterIndex, bytes, packets);
// }

// void
// P4Controller::SetMeterRates(uint32_t index,
//                             const std::string& tableName,
//                             bm::entry_handle_t handle,
//                             const std::vector<bm::Meter::rate_config_t>&
//                             configs)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }
//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_ERROR("No V1Model core found for switch " << index);
//         return;
//     }
//     core->SetMeterRates(tableName, handle, configs);
// }

// void
// P4Controller::GetMeterRates(uint32_t index, const std::string& tableName,
// bm::entry_handle_t handle)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }
//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_ERROR("No V1Model core found for switch " << index);
//         return;
//     }

//     std::vector<bm::Meter::rate_config_t> configs;
//     bm::MatchErrorCode rc = core->GetMeterRates(tableName, handle, &configs);
//     if (rc == bm::MatchErrorCode::SUCCESS)
//     {
//         for (const auto& conf : configs)
//         {
//             NS_LOG_INFO("Meter rate config - Cir: " << conf.info_rate
//                                                     << ", Cburst: " <<
//                                                     conf.burst_size);
//         }
//     }
// }

// void
// P4Controller::ResetMeterRates(uint32_t index,
//                               const std::string& tableName,
//                               bm::entry_handle_t handle)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }
//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_ERROR("No V1Model core found for switch " << index);
//         return;
//     }
//     core->ResetMeterRates(tableName, handle);
// }
// void
// P4Controller::MeterArraySetRates(uint32_t index,
//                                   const std::string& meterName,
//                                   const
//                                   std::vector<bm::Meter::rate_config_t>&
//                                   configs)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     core->SetMeterArrayRates(meterName, configs);
// }
// void
// P4Controller::MeterSetRates(uint32_t index,
//                              const std::string& meterName,
//                              size_t idx,
//                              const std::vector<bm::Meter::rate_config_t>&
//                              configs)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     core->SetMeterRates(meterName, idx, configs);
// }
// void
// P4Controller::MeterGetRates(uint32_t index,
//                              const std::string& meterName,
//                              size_t idx)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     std::vector<bm::Meter::rate_config_t> configs;
//     core->GetMeterRates(meterName, idx, &configs);
// }
// void
// P4Controller::MeterResetRates(uint32_t index,
//                                const std::string& meterName,
//                                size_t idx)
// {
//     if (index >= m_connectedSwitches.size())
//     {
//         NS_LOG_WARN("Invalid switch index " << index);
//         return;
//     }

//     P4CoreV1model* core = m_connectedSwitches[index]->GetV1ModelCore();
//     if (!core)
//     {
//         NS_LOG_WARN("No V1Model core found for switch " << index);
//         return;
//     }

//     core->ResetMeterRates(meterName, idx);
// }
void P4Controller::RegisterRead(uint32_t index, const std::string &registerName,
                                size_t regIndex, bm::Data *value) {
  NS_LOG_FUNCTION(this << index << registerName << regIndex);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->RegisterRead(registerName, regIndex, value);

  if (status == 0) {
    NS_LOG_INFO("RegisterRead succeeded: switch "
                << index << ", register [" << registerName << "], index "
                << regIndex << ", value = " << value);
  } else {
    NS_LOG_ERROR("RegisterRead failed for switch "
                 << index << ", register [" << registerName << "], index "
                 << regIndex);
  }
}

void P4Controller::RegisterWrite(uint32_t index,
                                 const std::string &registerName,
                                 size_t regIndex, const bm::Data &value) {
  NS_LOG_FUNCTION(this << index << registerName << regIndex << value);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->RegisterWrite(registerName, regIndex, value);

  if (status == 0) {
    NS_LOG_INFO("RegisterWrite succeeded: switch "
                << index << ", register [" << registerName << "], index "
                << regIndex << ", value = " << value);
  } else {
    NS_LOG_ERROR("RegisterWrite failed for switch "
                 << index << ", register [" << registerName << "], index "
                 << regIndex);
  }
}

void P4Controller::RegisterReadAll(uint32_t index,
                                   const std::string &registerName) {
  NS_LOG_FUNCTION(this << index << registerName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  std::vector<bm::Data> values = core->RegisterReadAll(registerName);

  if (!values.empty()) {
    NS_LOG_INFO("RegisterReadAll succeeded for register ["
                << registerName << "] on switch " << index
                << ". Number of values: " << values.size());
  } else {
    NS_LOG_ERROR("RegisterReadAll failed or returned empty for register ["
                 << registerName << "] on switch " << index);
  }
}

void P4Controller::RegisterWriteRange(uint32_t index,
                                      const std::string &registerName,
                                      size_t startIndex, size_t endIndex,
                                      const bm::Data &value) {
  NS_LOG_FUNCTION(this << index << registerName << startIndex << endIndex
                       << value);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status =
      core->RegisterWriteRange(registerName, startIndex, endIndex, value);

  if (status == 0) {
    NS_LOG_INFO("RegisterWriteRange succeeded: switch "
                << index << ", register [" << registerName << "], indices ["
                << startIndex << "-" << endIndex << "], value = " << value);
  } else {
    NS_LOG_ERROR("RegisterWriteRange failed for switch "
                 << index << ", register [" << registerName << "], indices ["
                 << startIndex << "-" << endIndex << "]");
  }
}

void P4Controller::RegisterReset(uint32_t index,
                                 const std::string &registerName) {
  NS_LOG_FUNCTION(this << index << registerName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
  P4CoreV1model *core = sw->GetV1ModelCore();

  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->RegisterReset(registerName);

  if (status == 0) {
    NS_LOG_INFO("RegisterReset succeeded for register ["
                << registerName << "] on switch " << index);
  } else {
    NS_LOG_ERROR("RegisterReset failed for register ["
                 << registerName << "] on switch " << index);
  }
}
void P4Controller::ParseVsetGet(uint32_t index, const std::string &vsetName) {
  NS_LOG_FUNCTION(this << index << vsetName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  std::vector<bm::ByteContainer> values;
  int status = core->ParseVsetGet(vsetName, &values);

  if (status == 0) {
    NS_LOG_INFO("ParseVsetGet succeeded for switch "
                << index << ", vset [" << vsetName
                << "], count = " << values.size());
    for (size_t i = 0; i < values.size(); ++i) {
      NS_LOG_INFO("  Value[" << i << "] = " << values[i].to_hex());
    }
  } else {
    NS_LOG_ERROR("ParseVsetGet failed for switch " << index << ", vset ["
                                                   << vsetName << "]");
  }
}
void P4Controller::ParseVsetAdd(uint32_t index, const std::string &vsetName,
                                const bm::ByteContainer &value) {
  NS_LOG_FUNCTION(this << index << vsetName << value.to_hex());

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ParseVsetAdd(vsetName, value);
  if (status == 0) {
    NS_LOG_INFO("ParseVsetAdd succeeded for switch "
                << index << ", vset [" << vsetName
                << "], value = " << value.to_hex());
  } else {
    NS_LOG_ERROR("ParseVsetAdd failed for switch " << index << ", vset ["
                                                   << vsetName << "]");
  }
}

void P4Controller::ParseVsetRemove(uint32_t index, const std::string &vsetName,
                                   const bm::ByteContainer &value) {
  NS_LOG_FUNCTION(this << index << vsetName << value.to_hex());

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ParseVsetRemove(vsetName, value);
  if (status == 0) {
    NS_LOG_INFO("ParseVsetRemove succeeded for switch "
                << index << ", vset [" << vsetName
                << "], value = " << value.to_hex());
  } else {
    NS_LOG_ERROR("ParseVsetRemove failed for switch " << index << ", vset ["
                                                      << vsetName << "]");
  }
}

void P4Controller::ParseVsetClear(uint32_t index, const std::string &vsetName) {
  NS_LOG_FUNCTION(this << index << vsetName);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  auto core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ParseVsetClear(vsetName);
  if (status == 0) {
    NS_LOG_INFO("ParseVsetClear succeeded for switch " << index << ", vset ["
                                                       << vsetName << "]");
  } else {
    NS_LOG_ERROR("ParseVsetClear failed for switch " << index << ", vset ["
                                                     << vsetName << "]");
  }
}

void P4Controller::ResetState(uint32_t index) {
  NS_LOG_FUNCTION(this << index);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->ResetState();
  if (status == 0) {
    NS_LOG_INFO("ResetState succeeded for switch " << index);
  } else {
    NS_LOG_ERROR("ResetState failed for switch " << index);
  }
}

void P4Controller::Serialize(uint32_t index, std::ostream *out) {
  NS_LOG_FUNCTION(this << index);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->Serialize(out);
  if (status == 0) {
    NS_LOG_INFO("Serialize succeeded for switch " << index);
  } else {
    NS_LOG_ERROR("Serialize failed for switch " << index);
  }
}

void P4Controller::LoadNewConfig(uint32_t index, const std::string &newConfig) {
  NS_LOG_FUNCTION(this << index);

  if (index >= m_connectedSwitches.size()) {
    NS_LOG_WARN("Invalid switch index " << index);
    return;
  }

  P4CoreV1model *core = m_connectedSwitches[index]->GetV1ModelCore();
  if (!core) {
    NS_LOG_ERROR("V1Model core not found for switch " << index);
    return;
  }

  int status = core->LoadNewConfig(newConfig);
  if (status == 0) {
    NS_LOG_INFO("LoadNewConfig succeeded for switch " << index);
  } else {
    NS_LOG_ERROR("LoadNewConfig failed for switch " << index);
  }
}

void P4Controller::SetP4SwitchViewFlowTablePath(
    size_t index, const std::string &viewFlowTablePath) {}

void P4Controller::SetP4SwitchFlowTablePath(size_t index,
                                            const std::string &flowTablePath) {
  NS_LOG_FUNCTION(this << index << flowTablePath);
}

} // namespace ns3