#ifndef P4_CONTROLLER_H
#define P4_CONTROLLER_H

#include "p4-switch-interface.h"
#include "ns3/object.h"

#include <string>
#include <vector>

namespace ns3 {

/**
 * @brief Controller for managing multiple P4 switches in an NS-3 simulation.
 * 
 * This class provides methods for:
 * - Viewing flow table information for all or specific P4 switches.
 * - Setting paths for flow table configuration and viewing.
 * - Managing a collection of P4 switches, including adding and retrieving switches.
 */
class P4Controller : public Object
{
public:
    /**
     * @brief Constructor for P4Controller.
     */
    P4Controller();

    /**
     * @brief Destructor for P4Controller.
     */
    ~P4Controller();

    /**
     * @brief View flow table information for all P4 switches managed by the controller.
     */
    void ViewAllSwitchFlowTableInfo();

    /**
     * @brief View detailed flow table, counter, register, and meter information 
     *        for a specific P4 switch.
     * 
     * @param index Index of the P4 switch in the controller's collection.
     */
    void ViewP4SwitchFlowTableInfo(size_t index);

    /**
     * @brief Set the path for viewing flow table information of a specific P4 switch.
     * 
     * @param index Index of the P4 switch in the controller's collection.
     * @param viewFlowTablePath File path for viewing flow table information.
     */
    void SetP4SwitchViewFlowTablePath(size_t index, const std::string& viewFlowTablePath);

    /**
     * @brief Set the path for populating the flow table of a specific P4 switch.
     * 
     * @param index Index of the P4 switch in the controller's collection.
     * @param flowTablePath File path for populating flow table entries.
     */
    void SetP4SwitchFlowTablePath(size_t index, const std::string& flowTablePath);

    /**
     * @brief Retrieve a specific P4 switch from the controller.
     * 
     * @param index Index of the P4 switch in the controller's collection.
     * @return Pointer to the requested P4SwitchInterface.
     */
    P4SwitchInterface* GetP4Switch(size_t index);

    /**
     * @brief Add a new P4 switch to the controller.
     * 
     * @return Pointer to the newly added P4SwitchInterface.
     */
    P4SwitchInterface* AddP4Switch();

    /**
     * @brief Get the total number of P4 switches managed by the controller.
     * 
     * @return Number of P4 switches.
     */
    unsigned int GetP4SwitchNum() const
    {
        return m_p4Switches.size();
    }

    /**
     * @brief Register the P4Controller class with the NS-3 type system.
     * 
     * @return TypeId for the P4Controller class.
     */
    static TypeId GetTypeId(void);

private:
    /**
     * @brief Collection of P4 switch interfaces managed by the controller.
     *        Each switch is identified by its index in this vector.
     */
    std::vector<P4SwitchInterface*> m_p4Switches;

    // Disable copy constructor and assignment operator
    P4Controller(const P4Controller&) = delete;
    P4Controller& operator=(const P4Controller&) = delete;
};

} // namespace ns3

#endif // P4_CONTROLLER_H
