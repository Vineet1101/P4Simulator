#ifndef EXCEPTION_HANDLE_H
#define EXCEPTION_HANDLE_H

#include <exception>
#include <string>
#include "ns3/log.h"

namespace ns3 {

// NS_LOG_COMPONENT_DEFINE("P4ExceptionHandle");

// Error code constants
constexpr unsigned int PARAMETER_NUM_ERROR = 0;
constexpr unsigned int MATCH_KEY_NUM_ERROR = 1;
constexpr unsigned int MATCH_KEY_TYPE_ERROR = 2;
constexpr unsigned int ACTION_DATA_NUM_ERROR = 3;
constexpr unsigned int ACTION_DATA_TYPE_ERROR = 4;
constexpr unsigned int NO_SUCCESS = 5;
constexpr unsigned int COMMAND_ERROR = 6;
constexpr unsigned int METER_NO_EXIST = 7;
constexpr unsigned int COUNTER_NO_EXIST = 8;
constexpr unsigned int P4_SWITCH_POINTER_NULL = 9;
constexpr unsigned int MATCH_TYPE_ERROR = 10;
constexpr unsigned int OTHER_ERROR = 20;

class P4Exception : public std::exception {
public:
    P4Exception(unsigned int code = OTHER_ERROR, const std::string& entry = "") 
        : m_exceptionCode(code), m_entry(entry) 
    {
        NS_LOG_ERROR("P4Exception thrown: Code = " << code << ", Entry = " << entry);
    }

    const char * what() const noexcept override {
        switch (m_exceptionCode) {
            case PARAMETER_NUM_ERROR: return "PARAMETER_NUM_ERROR";
            case MATCH_KEY_NUM_ERROR: return "MATCH_KEY_NUM_ERROR";
            case MATCH_KEY_TYPE_ERROR: return "MATCH_KEY_TYPE_ERROR";
            case ACTION_DATA_NUM_ERROR: return "ACTION_DATA_NUM_ERROR";
            case ACTION_DATA_TYPE_ERROR: return "ACTION_DATA_TYPE_ERROR";
            case NO_SUCCESS: return "NO_SUCCESS";
            case COMMAND_ERROR: return "COMMAND_ERROR";
            case METER_NO_EXIST: return "METER_NO_EXIST";
            case COUNTER_NO_EXIST: return "COUNTER_NO_EXIST";
            case P4_SWITCH_POINTER_NULL: return "P4_SWITCH_POINTER_NULL";
            case MATCH_TYPE_ERROR: return "MATCH_TYPE_ERROR";
            default: return "OTHER_ERROR";
        }
    }

    std::string info() const {
        return m_entry;
    }

    ~P4Exception() {}

private:
    unsigned int m_exceptionCode;
    std::string m_entry;
};

} // namespace ns3

#endif // EXCEPTION_HANDLE_H
