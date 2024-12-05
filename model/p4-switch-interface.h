#ifndef P4_SWITCH_INTERFACE_H
#define P4_SWITCH_INTERFACE_H

#include "ns3/bridge-p4-net-device.h"
#include "ns3/p4-controller.h"
#include "ns3/p4-switch-core.h"
#include "ns3/switch-api.h"
#include "ns3/global.h"

#include "ns3/object.h"



#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#include <bm/spdlog/spdlog.h>
#undef LOG_INFO // 防止冲突
#undef LOG_ERROR
#undef LOG_DEBUG

#include <bm/bm_sim/match_units.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns3
{

class P4Switch;

/**
 * \brief Represents a Meter configuration.
 */
struct Meter
{
    bool isDirect = false; //!< Indicates if this is a direct meter.
    std::string tableName; //!< The associated table name.

    Meter() = default;

    Meter(bool isDirect, const std::string& name = "")
        : isDirect(isDirect),
          tableName(name)
    {
    }
};

/**
 * \brief Represents a Counter configuration.
 */
struct Counter
{
    bool isDirect = false; //!< Indicates if this is a direct counter.
    std::string tableName; //!< The associated table name.

    Counter() = default;

    Counter(bool isDirect, const std::string& name = "")
        : isDirect(isDirect),
          tableName(name)
    {
    }
};

/**
 * \brief Represents a Flow Table configuration.
 */
struct FlowTable
{
    bm::MatchKeyParam::Type matchType; //!< Match type for the flow table.

    FlowTable() = default;

    explicit FlowTable(bm::MatchKeyParam::Type mt)
        : matchType(mt)
    {
    }
};

/**
 * \brief P4SwitchInterface manages the interaction with a P4-based switch.
 */
class P4SwitchInterface : public Object
{
  public:
    P4SwitchInterface();
    ~P4SwitchInterface() override;

    static TypeId GetTypeId();

    // Setters
    void SetP4NetDeviceCore(P4Switch* model)
    {
        p4Core_ = model;
    }

    void SetJsonPath(const std::string& path)
    {
        jsonPath_ = path;
    }

    void SetP4InfoPath(const std::string& path)
    {
        p4InfoPath_ = path;
    }

    void SetFlowTablePath(const std::string& path)
    {
        flowTablePath_ = path;
    }

    void SetViewFlowTablePath(const std::string& path)
    {
        viewFlowTablePath_ = path;
    }

    void SetNetworkFunc(unsigned int func)
    {
        networkFunc_ = func;
    }

    void SetPopulateFlowTableWay(uint way)
    {
        populateFLowTableWay_ = way;
    }

    // Getters
    P4Switch* GetP4Switch() const
    {
        return p4Core_;
    }

    const std::string& GetJsonPath() const
    {
        return jsonPath_;
    }

    const std::string& GetP4InfoPath() const
    {
        return p4InfoPath_;
    }

    const std::string& GetFlowTablePath() const
    {
        return flowTablePath_;
    }

    const std::string& GetViewFlowTablePath() const
    {
        return viewFlowTablePath_;
    }

    unsigned int GetNetworkFunc() const
    {
        return networkFunc_;
    }

    uint GetPopulateFlowTableWay() const
    {
        return populateFLowTableWay_;
    }

    // Flow table and switch operations
    void PopulateFlowTable();
    void ReadP4Info();
    void ViewFlowtableEntryNum();
    void AttainSwitchFlowTableInfo();

    void ParseAttainFlowTableInfoCommand(const std::string& commandRow);
    void ParsePopulateFlowTableCommand(const std::string& commandRow);

    // ParseAttainFlowTableInfoCommand helper functions
    void HandleTableDumpEntry(const std::vector<std::string>& parms);
    void HandleTableNumEntries(const std::vector<std::string>& parms);
    void HandleTableClear(const std::vector<std::string>& parms);
    void HandleMeterGetRates(const std::vector<std::string>& parms);
    void HandleCounterRead(const std::vector<std::string>& parms);
    void HandleCounterReset(const std::vector<std::string>& parms);
    void HandleRegisterRead(const std::vector<std::string>& parms);
    void HandleRegisterWrite(const std::vector<std::string>& parms);
    void HandleRegisterReset(const std::vector<std::string>& parms);
    void HandleTableDump(const std::vector<std::string>& parms);

    // ParsePopulateFlowTableCommand helper functions
    void HandleTableSetDefault(const std::vector<std::string>& parms);
    void HandleTableAdd(const std::vector<std::string>& parms);
    void HandleTableSetTimeout(const std::vector<std::string>& parms);
    void HandleTableModify(const std::vector<std::string>& parms);
    void HandleTableDelete(const std::vector<std::string>& parms);
    void HandleMeterArraySetRates(const std::vector<std::string>& parms);
    void HandleMeterSetRates(const std::vector<std::string>& parms);

    // helper functions
    bm::MatchKeyParam ParseMatchField(const std::string& field, bm::MatchKeyParam::Type matchType);
    bm::Meter::rate_config_t ParseRateConfig(const std::string& rateConfig);

    // Initialize the switch interface
    void Init();

  private:
    P4Switch* p4Core_ = nullptr;    //!< Pointer to the P4 core model.
    std::string jsonPath_;          //!< Path to the P4 JSON configuration file.
    std::string p4InfoPath_;        //!< Path to the P4 info file.
    std::string flowTablePath_;     //!< Path to the flow table file.
    std::string viewFlowTablePath_; //!< Path to the view flow table file.
    uint populateFLowTableWay_ = 0; //!< Method to populate the flow table.

    unsigned int networkFunc_ = 0;  //!< Network function ID.

    std::unordered_map<std::string, Meter> meters_;         //!< Map of meter configurations.
    std::unordered_map<std::string, FlowTable> flowTables_; //!< Map of flow table configurations.
    std::unordered_map<std::string, Counter> counters_;     //!< Map of counter configurations.

    // Disable copy constructor and assignment operator
    P4SwitchInterface(const P4SwitchInterface&) = delete;
    P4SwitchInterface& operator=(const P4SwitchInterface&) = delete;
};

} // namespace ns3

#endif // P4_SWITCH_INTERFACE_H
