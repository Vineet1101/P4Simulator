#include "ns3/p4-controller.h"

#include "ns3/log.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("P4Controller");
NS_OBJECT_ENSURE_REGISTERED(P4Controller);

TypeId
P4Controller::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::P4Controller").SetParent<Object>().SetGroupName("P4Controller");
    return tid;
}

P4Controller::P4Controller()
{
    NS_LOG_FUNCTION(this);
}

P4Controller::~P4Controller()
{
    NS_LOG_FUNCTION(this);
}

void
P4Controller::RegisterSwitch(ns3::Ptr<ns3::P4SwitchNetDevice> sw)
{
    m_connectedSwitches.push_back(sw);
    NS_LOG_INFO("Switch registered successfully");
}

uint32_t
P4Controller::GetN()
{
    return m_connectedSwitches.size();
}

void
P4Controller::ViewAllSwitchFlowTableInfo()
{
    NS_LOG_INFO("\n==== Viewing All P4 Switch Flow Tables ====\n");
    for (uint32_t i = 0; i < m_connectedSwitches.size(); ++i)
    {
        ViewP4SwitchFlowTableInfo(i);
    }
    NS_LOG_INFO("==========================================\n");
}

void
P4Controller::ViewP4SwitchFlowTableInfo(uint32_t index)
{
    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_ERROR("[ERROR] Invalid switch index: " << index << "\n");
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    std::string path = sw->GetFlowTablePath();
    std::ifstream file(path);

    if (!file.is_open())
    {
        NS_LOG_ERROR("[ERROR] Could not open flow table file at: " << path << "\n");
        return;
    }

    NS_LOG_INFO("Flow Table for Switch " << index << "\n");
    std::string line;
    while (std::getline(file, line))
    {
        NS_LOG_INFO("  " << line << "\n");
    }
    file.close();
}

void
P4Controller::PrintTableEntryCount(uint32_t index, const std::string& tableName)
{   
    
    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_WARN("Switch " << index << " has no v1model core (core is null)");
        return;
    }

    int num = core->GetNumEntries(tableName);
    NS_LOG_INFO("Switch " << index << " - Table [" << tableName << "] has " << num << " entries.");
}

void
P4Controller::ClearFlowTableEntries(uint32_t index, const std::string& tableName, bool resetDefault)
{
   
    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_WARN("Switch " << index << " has no v1model core (core is null)");
        return;
    }
    int rc = core->ClearFlowTableEntries(tableName, resetDefault);

    if (rc == 0)
    {
        NS_LOG_INFO("Successfully cleared entries in table [" <<tableName << "] on switch "
                                                              << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to clear entries in table [" << tableName << "] on switch "
                                                          << index);
    }
}

void
P4Controller::AddFlowEntry(uint32_t index,
                           const std::string& tableName,
                           const std::vector<bm::MatchKeyParam>& matchKey,
                           const std::string& actionName,
                           bm::ActionData&& actionData,
                           int priority)
{
    NS_LOG_FUNCTION(this << index << tableName << actionName << priority);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    bm::entry_handle_t handle;
    int result = core->AddFlowEntry(tableName,
                                    matchKey,
                                    actionName,
                                    std::move(actionData),
                                    &handle,
                                    priority);

    if (result == 0)
    {
        NS_LOG_INFO("Successfully added flow entry to table ["
                    << tableName << "] on switch " << index << " (handle = " << handle << ")");
    }
    else
    {
        NS_LOG_ERROR("Failed to add flow entry to table [" << tableName << "] on switch " << index
                                                           << ", result code = " << result);
    }
}
void 
P4Controller::SetDefaultAction(uint32_t index,
                                    const std::string& tableName,
                                    const std::string& actionName,
                                    bm::ActionData&& actionData)
{
    NS_LOG_FUNCTION(this << index << tableName << actionName);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    int status = core->SetDefaultAction(tableName, actionName, std::move(actionData));

    if (status == 0)
    {
        NS_LOG_INFO("Successfully set default action [" << actionName << "] for table ["
                     << tableName << "] on switch " << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to set default action for table [" << tableName << "] on switch "
                      << index << ", code = " << status);
    }
}
void 
P4Controller::ResetDefaultEntry(uint32_t index, const std::string& tableName)
{
    NS_LOG_FUNCTION(this << index << tableName);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();  

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    int status = core->ResetDefaultEntry(tableName);

    if (status == 0)
    {
        NS_LOG_INFO("Successfully reset default entry for table [" << tableName
                     << "] on switch " << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to reset default entry for table [" << tableName
                      << "] on switch " << index << ", code = " << status);
    }
}
void 
P4Controller::DeleteFlowEntry(uint32_t index, const std::string& tableName, bm::entry_handle_t handle)
{
    NS_LOG_FUNCTION(this << index << tableName << handle);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    int status = core->DeleteFlowEntry(tableName, handle);

    if (status == 0)
    {
        NS_LOG_INFO("Successfully deleted entry (handle = " << handle
                     << ") from table [" << tableName << "] on switch " << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to delete entry from table [" << tableName
                      << "] on switch " << index << ", code = " << status);
    }
}

void P4Controller::ModifyFlowEntry(uint32_t index,
                                   const std::string& tableName,
                                   bm::entry_handle_t handle,
                                   const std::string& actionName,
                                   bm::ActionData actionData)
{
    NS_LOG_FUNCTION(this << index << tableName << handle << actionName);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    int status = core->ModifyFlowEntry(tableName, handle, actionName, std::move(actionData));

    if (status == 0)
    {
        NS_LOG_INFO("Modified flow entry for handle " << handle << " in table [" << tableName << "] on switch " << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to modify flow entry for handle " << handle << " in table [" << tableName << "] on switch " << index);
    }
}
void
P4Controller::SetEntryTtl(uint32_t index,
                               const std::string& tableName,
                               bm::entry_handle_t handle,
                               unsigned int ttlMs)
{
    NS_LOG_FUNCTION(this << index << tableName << handle << ttlMs);

    if (index >= m_connectedSwitches.size())
    {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1model* core = sw->GetV1ModelCore();

    if (!core)
    {
        NS_LOG_ERROR("V1Model core not found for switch " << index);
        return;
    }

    int status = core->SetEntryTtl(tableName, handle, ttlMs);

    if (status == 0)
    {
        NS_LOG_INFO("Set TTL = " << ttlMs << "ms for entry handle " << handle << " in table [" << tableName << "] on switch " << index);
    }
    else
    {
        NS_LOG_ERROR("Failed to set TTL for entry handle " << handle << " in table [" << tableName << "] on switch " << index);
    }
}

void
P4Controller::SetP4SwitchViewFlowTablePath(size_t index, const std::string& viewFlowTablePath)
{
}

void
P4Controller::SetP4SwitchFlowTablePath(size_t index, const std::string& flowTablePath)
{
    NS_LOG_FUNCTION(this << index << flowTablePath);
}

} // namespace ns3