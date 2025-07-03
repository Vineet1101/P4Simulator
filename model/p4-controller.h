#ifndef P4_CONTROLLER_H
#define P4_CONTROLLER_H

#include "p4-switch-net-device.h"

#include "ns3/object.h"
#include "ns3/p4-core-v1model.h"
#include <ns3/network-module.h>

#include <string>
#include <vector>

namespace ns3 {

class P4CoreV1model;

/**
 * @brief Controller for managing multiple P4 switches in an NS-3 simulation.
 *
 * This class provides methods for:
 * - Viewing flow table information for all or specific P4 switches.
 * - Setting paths for flow table configuration and viewing.
 * - Managing a collection of P4 switches, including adding and retrieving
 * switches.
 */
class P4Controller : public Object {
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
   * @brief Register the P4Controller class with the NS-3 type system.
   *
   * @return TypeId for the P4Controller class.
   */
  static TypeId GetTypeId(void);
  /**
   * @brief Register P4 Switch with the controller.
   */
  void RegisterSwitch(ns3::Ptr<ns3::P4SwitchNetDevice> sw);

  /**
   * @brief Gives the count of the p4 switches registered with the controller
   *
   */
  uint32_t GetN();

  /**
   * @brief View flow table information for all P4 switches managed by the
   * controller.
   */
  void ViewAllSwitchFlowTableInfo();

  /**
   * @brief View detailed flow table, counter, register, and meter information
   *        for a specific P4 switch.
   *
   * @param index Index of the P4 switch in the controller's collection.
   */
  void ViewP4SwitchFlowTableInfo(uint32_t index);
  /**
   * @brief Set the path for viewing flow table information of a specific P4
   * switch.
   *
   * @param index Index of the P4 switch in the controller's collection.
   * @param viewFlowTablePath File path for viewing flow table information.
   */
  void SetP4SwitchViewFlowTablePath(size_t index,
                                    const std::string &viewFlowTablePath);

  /**
   * @brief Set the path for populating the flow table of a specific P4 switch.
   *
   * @param index Index of the P4 switch in the controller's collection.
   * @param flowTablePath File path for populating flow table entries.
   */
  void SetP4SwitchFlowTablePath(size_t index, const std::string &flowTablePath);

  void PrintTableEntryCount(uint32_t index, const std::string &tableName);
  /**
   *  @brief Clears the table entries from the given switch and table
   */
  void ClearFlowTableEntries(uint32_t index, const std::string &tableName,
                             bool resetDefault = false);
  void AddFlowEntry(uint32_t index, const std::string &tableName,
                    const std::vector<bm::MatchKeyParam> &matchKey,
                    const std::string &actionName, bm::ActionData &&actionData,
                    int priority = -1);
  void SetDefaultAction(uint32_t index, const std::string &tableName,
                        const std::string &actionName,
                        bm::ActionData &&actionData);
  void ResetDefaultEntry(uint32_t index, const std::string &tableName);
  void DeleteFlowEntry(uint32_t index, const std::string &tableName,
                       bm::entry_handle_t handle);
  void ModifyFlowEntry(uint32_t index, const std::string &tableName,
                       bm::entry_handle_t handle, const std::string &actionName,
                       bm::ActionData actionData);
  void SetEntryTtl(uint32_t index, const std::string &tableName,
                   bm::entry_handle_t handle, unsigned int ttlMs);

  // ======== Action Profile Operations ===========

  void AddActionProfileMember(uint32_t index, const std::string &profileName,
                              const std::string &actionName,
                              bm::ActionData &&actionData);

  void DeleteActionProfileMember(uint32_t index, const std::string &profileName,
                                 bm::ActionProfile::mbr_hdl_t memberHandle);

  void ModifyActionProfileMember(uint32_t index, const std::string &profileName,
                                 bm::ActionProfile::mbr_hdl_t memberHandle,
                                 const std::string &actionName,
                                 bm::ActionData &&actionData);
  /**
   * @brief Creates a new group in the specified action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param outHandle Pointer to the group handle that will be set on success
   */
  void CreateActionProfileGroup(uint32_t index, const std::string &profileName,
                                bm::ActionProfile::grp_hdl_t *outHandle);

  /**
   * @brief Deletes a group in the specified action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param groupHandle Handle of the group to be deleted
   */
  void DeleteActionProfileGroup(uint32_t index, const std::string &profileName,
                                bm::ActionProfile::grp_hdl_t groupHandle);

  /**
   * @brief Adds a member to a group in an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param memberHandle Handle of the member to add
   * @param groupHandle Handle of the group to which the member should be added
   */
  void AddMemberToGroup(uint32_t index, const std::string &profileName,
                        bm::ActionProfile::mbr_hdl_t memberHandle,
                        bm::ActionProfile::grp_hdl_t groupHandle);
  /**
   * @brief Removes a member from a group in an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param memberHandle Handle of the member to remove
   * @param groupHandle Handle of the group from which the member is removed
   */
  void RemoveMemberFromGroup(uint32_t index, const std::string &profileName,
                             bm::ActionProfile::mbr_hdl_t memberHandle,
                             bm::ActionProfile::grp_hdl_t groupHandle);

  /**
   * @brief Retrieves all members in an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   */
  void GetActionProfileMembers(uint32_t index, const std::string &profileName);

  /**
   * @brief Retrieves a specific member from an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param memberHandle Handle of the member to retrieve
   */
  void GetActionProfileMember(uint32_t index, const std::string &profileName,
                              bm::ActionProfile::mbr_hdl_t memberHandle);
  /**
   * @brief Retrieves all groups in an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   */
  void GetActionProfileGroups(uint32_t index, const std::string &profileName);

  /**
   * @brief Retrieves a specific group from an action profile.
   * @param index Index of the switch to operate on
   * @param profileName Name of the action profile
   * @param groupHandle Handle of the group to retrieve
   */
  void GetActionProfileGroup(uint32_t index, const std::string &profileName,
                             bm::ActionProfile::grp_hdl_t groupHandle);

  // ========== Indirect Table Operations ============
  /**
   * @brief Adds an entry to an indirect match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param matchKey Match key vector
   * @param memberHandle Handle of action profile member
   * @param outHandle Output entry handle
   * @param priority Optional priority value (default = 1)
   */
  void AddIndirectEntry(uint32_t index, const std::string &tableName,
                        const std::vector<bm::MatchKeyParam> &matchKey,
                        bm::ActionProfile::mbr_hdl_t memberHandle,
                        bm::entry_handle_t *outHandle, int priority = 1);

  /**
   * @brief Modifies an existing entry in an indirect match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param entryHandle Entry handle to modify
   * @param memberHandle New member handle
   */
  void ModifyIndirectEntry(uint32_t index, const std::string &tableName,
                           bm::entry_handle_t entryHandle,
                           bm::ActionProfile::mbr_hdl_t memberHandle);

  /**
   * @brief Deletes an entry from an indirect match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param entryHandle Handle of entry to delete
   */
  void DeleteIndirectEntry(uint32_t index, const std::string &tableName,
                           bm::entry_handle_t entryHandle);
  /**
   * @brief Sets the TTL (timeout in ms) for an indirect match table entry.
   * @param index Switch index
   * @param tableName Match table name
   * @param handle Entry handle
   * @param ttlMs Timeout in milliseconds
   */
  void SetIndirectEntryTtl(uint32_t index, const std::string &tableName,
                           bm::entry_handle_t handle, unsigned int ttlMs);

  /**
   * @brief Sets the default member for an indirect match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param memberHandle Member handle to set as default
   */
  void SetIndirectDefaultMember(uint32_t index, const std::string &tableName,
                                bm::ActionProfile::mbr_hdl_t memberHandle);

  /**
   * @brief Resets the default entry in an indirect match table.
   * @param index Switch index
   * @param tableName Match table name
   */
  void ResetIndirectDefaultEntry(uint32_t index, const std::string &tableName);

  /**
   * @brief Adds an entry to an indirect wide-switch match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param matchKey Match key vector
   * @param groupHandle Group handle to associate with the entry
   * @param outHandle Pointer to receive the assigned entry handle
   * @param priority Match priority (default = 1)
   */
  void AddIndirectWsEntry(uint32_t index, const std::string &tableName,
                          const std::vector<bm::MatchKeyParam> &matchKey,
                          bm::ActionProfile::grp_hdl_t groupHandle,
                          bm::entry_handle_t *outHandle, int priority = 1);
  /**
   * @brief Modifies an indirect WS entry to use a different group handle.
   * @param index Switch index
   * @param tableName Match table name
   * @param handle Entry handle to modify
   * @param groupHandle New group handle
   */
  void ModifyIndirectWsEntry(uint32_t index, const std::string &tableName,
                             bm::entry_handle_t handle,
                             bm::ActionProfile::grp_hdl_t groupHandle);

  /**
   * @brief Sets the default group for an indirect wide-switch match table.
   * @param index Switch index
   * @param tableName Match table name
   * @param groupHandle Group handle to use as default
   */
  void SetIndirectWsDefaultGroup(uint32_t index, const std::string &tableName,
                                 bm::ActionProfile::grp_hdl_t groupHandle);

  // ========= Flow Table Entry Retrieval Operations ========
  void PrintFlowEntries(uint32_t index, const std::string &tableName);
  void PrintIndirectFlowEntries(uint32_t index, const std::string &tableName);
  void PrintIndirectWsFlowEntries(uint32_t index, const std::string &tableName);
  void PrintEntry(uint32_t index, const std::string &tableName,
                  bm::entry_handle_t handle);
  void PrintDefaultEntry(uint32_t index, const std::string &tableName);
  void PrintIndirectDefaultEntry(uint32_t index, const std::string &tableName);
  void PrintIndirectWsDefaultEntry(uint32_t index,
                                   const std::string &tableName);
  void PrintEntryFromKey(uint32_t index, const std::string &tableName,
                         const std::vector<bm::MatchKeyParam> &matchKey);
  void
  PrintIndirectEntryFromKey(uint32_t index, const std::string &tableName,
                            const std::vector<bm::MatchKeyParam> &matchKey);
  void
  PrintIndirectWsEntryFromKey(uint32_t index, const std::string &tableName,
                              const std::vector<bm::MatchKeyParam> &matchKey);

  // counter APIs
  void ReadCounters(uint32_t index, const std::string &tableName,
                    bm::entry_handle_t handle);
  void ResetCounters(uint32_t index, const std::string &tableName);
  void WriteCounters(uint32_t index, const std::string &tableName,
                     bm::entry_handle_t handle, uint64_t bytes,
                     uint64_t packets);
  // void ReadCounter(uint32_t index, const std::string &counterName, size_t
  // counterIndex); void ResetCounter(uint32_t index, const std::string
  // &counterName); void WriteCounter(uint32_t index, const std::string
  // &counterName, size_t counterIndex,
  //                   bm::MatchTableAbstract::counter_value_t bytes,
  //                   bm::MatchTableAbstract::counter_value_t packets);

  // // ======= Meter Functions  =======
  // void SetMeterRates(uint32_t index, const std::string &tableName,
  //                    bm::entry_handle_t handle,
  //                    const std::vector<bm::Meter::rate_config_t> &configs);

  // void GetMeterRates(uint32_t index, const std::string &tableName,
  //                    bm::entry_handle_t handle);

  // void ResetMeterRates(uint32_t index, const std::string &tableName,
  //                      bm::entry_handle_t handle);

  // ======= Register  Functions  =======
  void RegisterRead(uint32_t index, const std::string &registerName,
                    size_t regIndex, bm::Data *value);

  void RegisterWrite(uint32_t index, const std::string &registerName,
                     size_t regIndex, const bm::Data &value);

  void RegisterReadAll(uint32_t index, const std::string &registerName);

  void RegisterWriteRange(uint32_t index, const std::string &registerName,
                          size_t startIndex, size_t endIndex,
                          const bm::Data &value);

  void RegisterReset(uint32_t index, const std::string &registerName);

  // ======= Parse Value Set Functions  =======
  void ParseVsetAdd(uint32_t index, const std::string &vsetName,
                    const bm::ByteContainer &value);
  void ParseVsetRemove(uint32_t index, const std::string &vsetName,
                       const bm::ByteContainer &value);
  void ParseVsetGet(uint32_t index, const std::string &vsetName);
  void ParseVsetClear(uint32_t index, const std::string &vsetName);

  // ======== Runtime State Management Functions =======

  void ResetState(uint32_t index);

  void Serialize(uint32_t index, std::ostream *out);

  void LoadNewConfig(uint32_t index, const std::string &newConfig);

  void SwapConfigs(uint32_t index);

  void GetConfig(uint32_t index);

  void GetConfigMd5(uint32_t index);

private:
  /**
   * @brief Collection of P4 switch interfaces managed by the controller.
   *        Each switch is identified by its index in this vector.
   */
  //   std::vector<P4SwitchInterface *> p4SwitchInterfaces_;

  // Disable copy constructor and assignment operator
  P4Controller(const P4Controller &) = delete;
  P4Controller &operator=(const P4Controller &) = delete;

  std::vector<ns3::Ptr<ns3::P4SwitchNetDevice>> m_connectedSwitches;
};

} // namespace ns3

#endif // P4_CONTROLLER_H
