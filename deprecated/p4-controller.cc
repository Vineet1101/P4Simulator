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

  // Clean up all P4SwitchInterface pointers
  for (size_t i = 0; i < p4SwitchInterfaces_.size (); i++)
    {
      if (p4SwitchInterfaces_[i] != nullptr)
        {
          delete p4SwitchInterfaces_[i];
          p4SwitchInterfaces_[i] = nullptr;
        }
    }
}

void
P4Controller::ViewAllSwitchFlowTableInfo ()
{
  NS_LOG_FUNCTION (this);

  for (size_t i = 0; i < p4SwitchInterfaces_.size (); i++)
    {
      ViewP4SwitchFlowTableInfo (i);
    }
}

void
P4Controller::ViewP4SwitchFlowTableInfo (size_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (index >= p4SwitchInterfaces_.size () || p4SwitchInterfaces_[index] == nullptr)
    {
      NS_LOG_ERROR ("Call ViewP4SwitchFlowTableInfo("
                    << index << "): P4SwitchInterface pointer is null or index out of range.");
      return;
    }

  p4SwitchInterfaces_[index]->AttainSwitchFlowTableInfo ();
}

void
P4Controller::SetP4SwitchViewFlowTablePath (size_t index, const std::string &viewFlowTablePath)
{
  NS_LOG_FUNCTION (this << index << viewFlowTablePath);

  if (index >= p4SwitchInterfaces_.size () || p4SwitchInterfaces_[index] == nullptr)
    {
      NS_LOG_ERROR ("Call SetP4SwitchViewFlowTablePath("
                    << index << "): P4SwitchInterface pointer is null or index out of range.");
      return;
    }

  p4SwitchInterfaces_[index]->SetViewFlowTablePath (viewFlowTablePath);
}

void
P4Controller::SetP4SwitchFlowTablePath (size_t index, const std::string &flowTablePath)
{
  NS_LOG_FUNCTION (this << index << flowTablePath);

  if (index >= p4SwitchInterfaces_.size () || p4SwitchInterfaces_[index] == nullptr)
    {
      NS_LOG_ERROR ("Call SetP4SwitchFlowTablePath("
                    << index << "): P4SwitchInterface pointer is null or index out of range.");
      return;
    }

  p4SwitchInterfaces_[index]->SetFlowTablePath (flowTablePath);
}

P4SwitchInterface *
P4Controller::GetP4Switch (size_t index)
{
  NS_LOG_FUNCTION (this << index);

  if (index >= p4SwitchInterfaces_.size ())
    {
      NS_LOG_ERROR ("Call GetP4Switch(" << index << "): Index out of range.");
      return nullptr;
    }

  return p4SwitchInterfaces_[index];
}

P4SwitchInterface *
P4Controller::AddP4Switch ()
{
  NS_LOG_FUNCTION (this);

  P4SwitchInterface *p4SwitchInterface = new P4SwitchInterface;
  p4SwitchInterfaces_.push_back (p4SwitchInterface);

  NS_LOG_INFO ("Added a new P4 switch. Total switches: " << p4SwitchInterfaces_.size ());
  return p4SwitchInterface;
}

} // namespace ns3
