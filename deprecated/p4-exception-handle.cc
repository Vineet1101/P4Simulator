#include "ns3/p4-exception-handle.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4ExceptionHandle");

P4Exception::P4Exception (P4ErrorCode code, const std::string &entry)
    : m_exceptionCode (code), m_entry (entry)
{
  NS_LOG_ERROR ("P4Exception thrown: Code = " << static_cast<unsigned int> (code)
                                              << ", Entry = " << entry);
}

const char *
P4Exception::what () const noexcept
{
  switch (m_exceptionCode)
    {
    case P4ErrorCode::PARAMETER_NUM_ERROR:
      return "PARAMETER_NUM_ERROR";
    case P4ErrorCode::MATCH_KEY_NUM_ERROR:
      return "MATCH_KEY_NUM_ERROR";
    case P4ErrorCode::MATCH_KEY_TYPE_ERROR:
      return "MATCH_KEY_TYPE_ERROR";
    case P4ErrorCode::ACTION_DATA_NUM_ERROR:
      return "ACTION_DATA_NUM_ERROR";
    case P4ErrorCode::ACTION_DATA_TYPE_ERROR:
      return "ACTION_DATA_TYPE_ERROR";
    case P4ErrorCode::NO_SUCCESS:
      return "NO_SUCCESS";
    case P4ErrorCode::COMMAND_ERROR:
      return "COMMAND_ERROR";
    case P4ErrorCode::METER_NO_EXIST:
      return "METER_NO_EXIST";
    case P4ErrorCode::COUNTER_NO_EXIST:
      return "COUNTER_NO_EXIST";
    case P4ErrorCode::P4_SWITCH_POINTER_NULL:
      return "P4_SWITCH_POINTER_NULL";
    case P4ErrorCode::MATCH_TYPE_ERROR:
      return "MATCH_TYPE_ERROR";
    default:
      return "OTHER_ERROR";
    }
}

std::string
P4Exception::info () const
{
  return m_entry;
}

void
P4Exception::ShowExceptionEntry (const std::string &entry) const
{
  NS_LOG_WARN ("Exception Entry: " << entry << " | Exception Code: "
                                   << static_cast<unsigned int> (m_exceptionCode)
                                   << " | Description: " << what ());
}

} // namespace ns3
