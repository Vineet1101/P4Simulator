#include "p4-exception-handle.h"
#include "ns3/log.h"
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("P4ExceptionHandle");

P4Exception::P4Exception(unsigned int code, const std::string& entry)
    : m_exceptionCode(code), m_entry(entry) {
    NS_LOG_ERROR("P4Exception thrown: Code = " << code << ", Entry = " << entry);
}


} // namespace ns3
