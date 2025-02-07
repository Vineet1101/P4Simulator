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
#undef LOG_INFO
#undef LOG_ERROR
#undef LOG_DEBUG

#include <bm/bm_sim/match_units.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns3 {

class P4Switch;

/**
 * \brief Represents a Meter configuration.
 */
struct Meter
{
  bool isDirect = false; //!< Indicates if this is a direct meter.
  std::string tableName; //!< Associated table name.

  Meter () = default;
  Meter (bool isDirect, const std::string &name = "") : isDirect (isDirect), tableName (name)
  {
  }
};

/**
 * \brief Represents a Counter configuration.
 */
struct Counter
{
  bool isDirect = false; //!< Indicates if this is a direct counter.
  std::string tableName; //!< Associated table name.

  Counter () = default;
  Counter (bool isDirect, const std::string &name = "") : isDirect (isDirect), tableName (name)
  {
  }
};

/**
 * \brief Represents a Flow Table configuration.
 */
struct FlowTable
{
  bm::MatchKeyParam::Type matchType; //!< Match type for the flow table.

  FlowTable () = default;
  explicit FlowTable (bm::MatchKeyParam::Type mt) : matchType (mt)
  {
  }
};

/**
 * \brief P4SwitchInterface manages the interaction with a P4-based switch.
 */
class P4SwitchInterface : public Object
{
public:
  // === Constructor & Destructor ===
  P4SwitchInterface ();
  ~P4SwitchInterface () override;

  // === NS-3 TypeId System ===
  static TypeId GetTypeId ();

  // === Setter Methods ===
  void SetP4NetDeviceCore (P4Switch *model);
  void SetJsonPath (const std::string &path);
  void SetP4InfoPath (const std::string &path);
  void SetFlowTablePath (const std::string &path);
  void SetViewFlowTablePath (const std::string &path);
  void SetNetworkFunc (unsigned int func);
  void SetPopulateFlowTableWay (uint way);
  // === Getter Methods ===
  P4Switch *GetP4Switch () const;
  const std::string &GetJsonPath () const;
  const std::string &GetP4InfoPath () const;
  const std::string &GetFlowTablePath () const;
  const std::string &GetViewFlowTablePath () const;
  unsigned int GetNetworkFunc () const;
  uint GetPopulateFlowTableWay () const;

  // === Flow Table Operations ===
  void PopulateFlowTable ();
  void ReadP4Info ();
  void ViewFlowtableEntryNum ();
  void AttainSwitchFlowTableInfo ();

  void ParseAttainFlowTableInfoCommand (const std::string &commandRow);
  void ParsePopulateFlowTableCommand (const std::string &commandRow);

  // Helper functions for `ParseAttainFlowTableInfoCommand`
  void HandleTableDumpEntry (const std::vector<std::string> &params);
  void HandleTableNumEntries (const std::vector<std::string> &params);
  void HandleTableClear (const std::vector<std::string> &params);
  void HandleMeterGetRates (const std::vector<std::string> &params);
  void HandleCounterRead (const std::vector<std::string> &params);
  void HandleCounterReset (const std::vector<std::string> &params);
  void HandleRegisterRead (const std::vector<std::string> &params);
  void HandleRegisterWrite (const std::vector<std::string> &params);
  void HandleRegisterReset (const std::vector<std::string> &params);
  void HandleTableDump (const std::vector<std::string> &params);

  // Helper functions for `ParsePopulateFlowTableCommand`
  void HandleTableSetDefault (const std::vector<std::string> &params);
  void HandleTableAdd (const std::vector<std::string> &params);
  void HandleTableSetTimeout (const std::vector<std::string> &params);
  void HandleTableModify (const std::vector<std::string> &params);
  void HandleTableDelete (const std::vector<std::string> &params);
  void HandleMeterArraySetRates (const std::vector<std::string> &params);
  void HandleMeterSetRates (const std::vector<std::string> &params);

  // Utility functions
  bm::MatchKeyParam ParseMatchField (const std::string &field, bm::MatchKeyParam::Type matchType);
  bm::Meter::rate_config_t ParseRateConfig (const std::string &rateConfig);

  /**
   * \brief Initialize the P4 switch core, based on the flow table population method.
   */
  void Init ();

private:
  // === Member Variables ===
  P4Switch *p4Core_ = nullptr; //!< Pointer to the P4 core model.
  std::string jsonPath_; //!< Path to the P4 JSON configuration file.
  std::string p4InfoPath_; //!< Path to the P4 info file.
  std::string flowTablePath_; //!< Path to the flow table file.
  std::string viewFlowTablePath_; //!< Path to the view flow table file.
  uint populateFLowTableWay_ = 0; //!< Method to populate the flow table.
  unsigned int networkFunc_ = 0; //!< Network function ID.

  std::unordered_map<std::string, Meter> meters_; //!< Map of meter configurations.
  std::unordered_map<std::string, FlowTable> flowTables_; //!< Map of flow table configurations.
  std::unordered_map<std::string, Counter> counters_; //!< Map of counter configurations.

  // === Disable Copy and Move ===
  P4SwitchInterface (const P4SwitchInterface &) = delete;
  P4SwitchInterface &operator= (const P4SwitchInterface &) = delete;
};

} // namespace ns3

#endif // P4_SWITCH_INTERFACE_H
