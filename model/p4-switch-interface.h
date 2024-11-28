#ifndef P4_SWITCH_INTERFACE_H
#define P4_SWITCH_INTERFACE_H

#include "p4-switch-core.h"
#include "bridge-p4-net-device.h"
#include "switch-api.h"
#include "p4-controller.h"
#include "ns3/object.h"

#include <bm/bm_sim/match_units.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace ns3 {

class P4Switch;

/**
 * \brief Represents a Meter configuration.
 */
struct Meter {
    bool isDirect = false;                 //!< Indicates if this is a direct meter.
    std::string tableName;                 //!< The associated table name.

    Meter() = default;
    Meter(bool isDirect, const std::string& name = "") 
        : isDirect(isDirect), tableName(name) {}
};

/**
 * \brief Represents a Counter configuration.
 */
struct Counter {
    bool isDirect = false;                 //!< Indicates if this is a direct counter.
    std::string tableName;                 //!< The associated table name.

    Counter() = default;
    Counter(bool isDirect, const std::string& name = "") 
        : isDirect(isDirect), tableName(name) {}
};

/**
 * \brief Represents a Flow Table configuration.
 */
struct FlowTable {
    bm::MatchKeyParam::Type matchType;     //!< Match type for the flow table.

    FlowTable() = default;
    explicit FlowTable(bm::MatchKeyParam::Type mt) : matchType(mt) {}
};

/**
 * \brief P4SwitchInterface manages the interaction with a P4-based switch.
 */
class P4SwitchInterface : public Object {
public:
    P4SwitchInterface();
    ~P4SwitchInterface() override;

    static TypeId GetTypeId();

    // Setters
    void SetP4NetDeviceCore(P4Switch* model) { p4Core_ = model; }
    void SetJsonPath(const std::string& path) { jsonPath_ = path; }
    void SetFlowTablePath(const std::string& path) { flowTablePath_ = path; }
    void SetViewFlowTablePath(const std::string& path) { viewFlowTablePath_ = path; }
    void SetNetworkFunc(unsigned int func) { networkFunc_ = func; }

    // Getters
    P4Switch* GetP4Switch() const { return p4Core_; }
    const std::string& GetJsonPath() const { return jsonPath_; }
    const std::string& GetP4InfoPath() const { return p4InfoPath_; }
    const std::string& GetFlowTablePath() const { return flowTablePath_; }
    const std::string& GetViewFlowTablePath() const { return viewFlowTablePath_; }
    unsigned int GetNetworkFunc() const { return networkFunc_; }

    // Flow table and switch operations
    void PopulateFlowTable();
    void ReadP4Info();
    void ViewFlowtableEntryNum();
    void AttainSwitchFlowTableInfo();
    void ParseAttainFlowTableInfoCommand(const std::string& commandRow);
    void ParsePopulateFlowTableCommand(const std::string& commandRow);

    // Initialize the switch interface
    void Init();

private:
    P4Switch* p4Core_ = nullptr;                  //!< Pointer to the P4 core model.
    std::string jsonPath_;                        //!< Path to the P4 JSON configuration file.
    std::string p4InfoPath_;                      //!< Path to the P4 info file.
    std::string flowTablePath_;                   //!< Path to the flow table file.
    std::string viewFlowTablePath_;               //!< Path to the view flow table file.
    unsigned int networkFunc_ = 0;                //!< Network function ID.

    std::unordered_map<std::string, Meter> meters_;          //!< Map of meter configurations.
    std::unordered_map<std::string, FlowTable> flowTables_;  //!< Map of flow table configurations.
    std::unordered_map<std::string, Counter> counters_;      //!< Map of counter configurations.

    // Disable copy constructor and assignment operator
    P4SwitchInterface(const P4SwitchInterface&) = delete;
    P4SwitchInterface& operator=(const P4SwitchInterface&) = delete;
};

} // namespace ns3

#endif // P4_SWITCH_INTERFACE_H
