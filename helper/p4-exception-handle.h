#ifndef EXCEPTION_HANDLE_H
#define EXCEPTION_HANDLE_H

#include <exception>
#include <string>
#include "ns3/log.h"

namespace ns3 {

enum class P4ErrorCode : unsigned int {
    PARAMETER_NUM_ERROR = 0,
    MATCH_KEY_NUM_ERROR = 1,
    MATCH_KEY_TYPE_ERROR = 2,
    ACTION_DATA_NUM_ERROR = 3,
    ACTION_DATA_TYPE_ERROR = 4,
    NO_SUCCESS = 5,
    COMMAND_ERROR = 6,
    METER_NO_EXIST = 7,
    COUNTER_NO_EXIST = 8,
    P4_SWITCH_POINTER_NULL = 9,
    MATCH_TYPE_ERROR = 10,
    OTHER_ERROR = 20
};

class P4Exception : public std::exception {
public:
    P4Exception(P4ErrorCode code = P4ErrorCode::OTHER_ERROR, const std::string& entry = "");
    
    const char* what() const noexcept override;

    std::string info() const;

    void ShowExceptionEntry(const std::string& entry) const;

    ~P4Exception() = default;

private:
    P4ErrorCode m_exceptionCode;
    std::string m_entry;
};

} // namespace ns3

#endif // EXCEPTION_HANDLE_H
