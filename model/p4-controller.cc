#include "ns3/p4-controller.h"
#include "ns3/log.h"
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4Controller");
NS_OBJECT_ENSURE_REGISTERED (P4Controller);


TypeId
P4Controller::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::P4Controller").SetParent<Object> ().SetGroupName ("P4Controller");
  return tid;
}

P4Controller::P4Controller ()
{
  NS_LOG_FUNCTION (this);  
}

P4Controller::~P4Controller ()
{
  NS_LOG_FUNCTION (this);
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
P4Controller::ViewAllSwitchFlowTableInfo ()
{
    NS_LOG_INFO("\n==== Viewing All P4 Switch Flow Tables ====\n");
    for (uint32_t i = 0; i < m_connectedSwitches.size(); ++i) {
        ViewP4SwitchFlowTableInfo(i);
    }
    NS_LOG_INFO("==========================================\n");
}

void
P4Controller::ViewP4SwitchFlowTableInfo (uint32_t index)
{
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

void 
P4Controller::PrintTableEntryCount(uint32_t index, const std::string& tableName)
{

    if (index >= m_connectedSwitches.size()) {
        NS_LOG_WARN("Invalid switch index " << index);
        return;
    }

    Ptr<P4SwitchNetDevice> sw = m_connectedSwitches[index];
    P4CoreV1Model* core = sw->GetV1ModelCore();

    if (!core) {
        NS_LOG_WARN("Switch " << index << " has no v1model core (core is null)");
        return;
    }

    int num = core->GetNumEntries(tableName);
    NS_LOG_INFO("Switch " << index << " - Table [" << tableName << "] has " << num << " entries.");


}

void
P4Controller::SetP4SwitchViewFlowTablePath (size_t index, const std::string &viewFlowTablePath)
{

}

void
P4Controller::SetP4SwitchFlowTablePath (size_t index, const std::string &flowTablePath)
{
  NS_LOG_FUNCTION (this << index << flowTablePath);

}

// P4SwitchInterface *
// P4Controller::GetP4Switch (size_t index)
// {
//   NS_LOG_FUNCTION (this << index);

//   // if (index >= p4SwitchInterfaces_.size ())
//   //   {
//   //     NS_LOG_ERROR ("Call GetP4Switch(" << index << "): Index out of range.");
//   //     return nullptr;
//   //   }

//   // return p4SwitchInterfaces_[index];
// }

// P4SwitchInterface *
// P4Controller::AddP4Switch ()
// {
//   NS_LOG_FUNCTION (this);

//   P4SwitchInterface *p4SwitchInterface = new P4SwitchInterface;
//   p4SwitchInterfaces_.push_back (p4SwitchInterface);

//   NS_LOG_INFO ("Added a new P4 switch. Total switches: " << p4SwitchInterfaces_.size ());
//   return p4SwitchInterface;
// }

} // namespace ns3