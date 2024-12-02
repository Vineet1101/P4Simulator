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

P4Controller::P4Controller() {
    NS_LOG_FUNCTION(this); // Log constructor call
}

P4Controller::~P4Controller() {
    NS_LOG_FUNCTION(this); // Log destructor call

    // Clean up all P4SwitchInterface pointers
    for (size_t i = 0; i < m_p4Switches.size(); i++) {
        if (m_p4Switches[i] != nullptr) {
            delete m_p4Switches[i];
            m_p4Switches[i] = nullptr; // Prevent dangling pointers
        }
    }
}

void P4Controller::ViewAllSwitchFlowTableInfo() {
    NS_LOG_FUNCTION(this); // Log function call

    // Iterate over all P4 switches and view their flow table info
    for (size_t i = 0; i < m_p4Switches.size(); i++) {
        ViewP4SwitchFlowTableInfo(i);
    }
}

void P4Controller::ViewP4SwitchFlowTableInfo(size_t index) {
    NS_LOG_FUNCTION(this << index); // Log function call with index

    if (index >= m_p4Switches.size() || m_p4Switches[index] == nullptr) {
        NS_LOG_ERROR("Call ViewP4SwitchFlowTableInfo(" << index
                     << "): P4SwitchInterface pointer is null or index out of range.");
        return;
    }

    // View flow table, counter, register, and meter info for the specific P4 switch
    m_p4Switches[index]->AttainSwitchFlowTableInfo();
}

void P4Controller::SetP4SwitchViewFlowTablePath(size_t index, const std::string &viewFlowTablePath) {
    NS_LOG_FUNCTION(this << index << viewFlowTablePath); // Log function call with parameters

    if (index >= m_p4Switches.size() || m_p4Switches[index] == nullptr) {
        NS_LOG_ERROR("Call SetP4SwitchViewFlowTablePath(" << index
                     << "): P4SwitchInterface pointer is null or index out of range.");
        return;
    }

    // Set the path for viewing flow table information
    m_p4Switches[index]->SetViewFlowTablePath(viewFlowTablePath);
}

void P4Controller::SetP4SwitchFlowTablePath(size_t index, const std::string &flowTablePath) {
    NS_LOG_FUNCTION(this << index << flowTablePath); // Log function call with parameters

    if (index >= m_p4Switches.size() || m_p4Switches[index] == nullptr) {
        NS_LOG_ERROR("Call SetP4SwitchFlowTablePath(" << index
                     << "): P4SwitchInterface pointer is null or index out of range.");
        return;
    }

    // Set the path for populating the flow table
    m_p4Switches[index]->SetFlowTablePath(flowTablePath);
}

P4SwitchInterface* P4Controller::GetP4Switch(size_t index) {
    NS_LOG_FUNCTION(this << index); // Log function call with index

    if (index >= m_p4Switches.size()) {
        NS_LOG_ERROR("Call GetP4Switch(" << index
                     << "): Index out of range.");
        return nullptr;
    }

    // Return the requested P4 switch
    return m_p4Switches[index];
}

P4SwitchInterface* P4Controller::AddP4Switch() {
    NS_LOG_FUNCTION(this); // Log function call

    //@TODO connect with the p4-bridge-net-device class

    // Create a new P4 switch and add it to the collection
    P4SwitchInterface* p4Switch = new P4SwitchInterface;
    m_p4Switches.push_back(p4Switch);

    NS_LOG_INFO("Added a new P4 switch. Total switches: " << m_p4Switches.size());
    return p4Switch;
}

} // namespace ns3
