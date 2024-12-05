/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/format-utils.h"

#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("P4FormatUtils");

char*
IntToStr(int num)
{
    NS_LOG_FUNCTION(num);
    try
    {
        char* ss = new char[12]; // Enough to hold int max value and sign
        int pos = 0;

        if (num == 0)
        {
            ss[0] = '0';
            ss[1] = '\0';
            return ss;
        }

        bool isNegative = (num < 0);
        if (isNegative)
        {
            num = -num;
        }

        while (num)
        {
            ss[pos++] = num % 10 + '0';
            num = num / 10;
        }

        if (isNegative)
        {
            ss[pos++] = '-';
        }

        std::reverse(ss, ss + pos);
        ss[pos] = '\0';
        return ss;
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in IntToStr: " << e.what());
        return nullptr;
    }
}

unsigned int
StrToInt(const std::string& str)
{
    NS_LOG_FUNCTION(str);
    unsigned int res = 0;
    try
    {
        if (str.find("0x") == 0)
        { // Hexadecimal
            for (size_t i = 2; i < str.size(); i++)
            {
                char c = str[i];
                if (isdigit(c))
                {
                    res = res * 16 + (c - '0');
                }
                else if (isxdigit(c))
                {
                    res = res * 16 + (tolower(c) - 'a' + 10);
                }
                else
                {
                    NS_LOG_ERROR("Invalid character in hex string: " << c);
                    throw std::invalid_argument("Invalid hex string");
                }
            }
        }
        else if (str.find("0b") == 0)
        { // Binary
            for (size_t i = 2; i < str.size(); i++)
            {
                char c = str[i];
                if (c == '0' || c == '1')
                {
                    res = res * 2 + (c - '0');
                }
                else
                {
                    NS_LOG_ERROR("Invalid character in binary string: " << c);
                    throw std::invalid_argument("Invalid binary string");
                }
            }
        }
        else
        { // Decimal
            for (size_t i = 0; i < str.size(); i++)
            {
                char c = str[i];
                if (isdigit(c))
                {
                    res = res * 10 + (c - '0');
                }
                else
                {
                    NS_LOG_ERROR("Invalid character in decimal string: " << c);
                    throw std::invalid_argument("Invalid decimal string");
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in StrToInt: " << e.what());
        res = 0;
    }
    return res;
}

int
HexcharToInt(char c)
{
    NS_LOG_FUNCTION(c);
    try
    {
        if (isdigit(c))
        {
            return c - '0';
        }
        else if (isxdigit(c))
        {
            return tolower(c) - 'a' + 10;
        }
        else
        {
            NS_LOG_ERROR("Invalid hexadecimal character: " << c);
            throw std::invalid_argument("Invalid hexadecimal character");
        }
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in HexcharToInt: " << e.what());
        return -1; // Return an invalid value for error
    }
}

std::string
HexStrToBytes(const std::string& str)
{
    NS_LOG_FUNCTION(str);
    try
    {
        std::string hexStr = (str.find("0x") == 0) ? str.substr(2) : str;
        if (hexStr.size() % 2 != 0)
        {
            NS_LOG_ERROR("Hex string length must be even.");
            throw std::invalid_argument("Hex string length must be even");
        }

        std::string res(hexStr.size() / 2, '\0');
        for (size_t i = 0; i < hexStr.size(); i += 2)
        {
            res[i / 2] = HexcharToInt(hexStr[i]) * 16 + HexcharToInt(hexStr[i + 1]);
        }
        return res;
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in HexstrToBytes: " << e.what());
        return "";
    }
}

std::string HexStrToBytes(const std::string& str, unsigned int bitWidth) {
    // Ensure bitWidth is a multiple of 8
    if (bitWidth % 8 != 0) {
        throw std::invalid_argument("bitWidth must be a multiple of 8.");
    }

    // Determine the maximum number of bytes based on bitWidth
    unsigned int maxBytes = bitWidth / 8;

    // Remove "0x" prefix if present
    std::string hexStr = str;
    if (hexStr.rfind("0x", 0) == 0) { // Check if it starts with "0x"
        hexStr = hexStr.substr(2); // Remove the "0x"
    }

    // Ensure the string length is even
    if (hexStr.length() % 2 != 0) {
        hexStr = "0" + hexStr; // Pad with a leading zero if necessary
    }

    // Convert hex string to bytes
    std::string result;
    result.reserve(maxBytes); // Pre-allocate space for the result
    unsigned int byteCount = 0;

    for (size_t i = 0; i < hexStr.length() && byteCount < maxBytes; i += 2) {
        // Check if the characters are valid hexadecimal digits
        if (!std::isxdigit(hexStr[i]) || !std::isxdigit(hexStr[i + 1])) {
            throw std::invalid_argument("Invalid hexadecimal character in input string.");
        }

        // Convert each pair of hex digits to a byte
        std::stringstream ss;
        ss << std::hex << hexStr.substr(i, 2);
        unsigned int byte;
        ss >> byte;
        result.push_back(static_cast<char>(byte));

        ++byteCount;
    }

    return result;
}

std::string
UintToString(unsigned int num)
{
    NS_LOG_FUNCTION(num);
    std::ostringstream oss;
    oss << num;
    return oss.str();
}

std::string
Uint32ipToHex(unsigned int ip)
{
    NS_LOG_FUNCTION(ip);
    std::ostringstream oss;
    oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << ip;
    return oss.str();
}

double
StrToDouble(const std::string& str)
{
    NS_LOG_FUNCTION(str);
    try
    {
        return std::stod(str);
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in StrToDouble: " << e.what());
        return 0.0;
    }
}

std::string
IpStrToBytes(const std::string& str)
{
    NS_LOG_FUNCTION(str);
    try
    {
        std::string res(4, '\0');
        size_t start = 0, end = 0;
        for (int i = 0; i < 4; i++)
        {
            end = str.find('.', start);
            if (end == std::string::npos && i != 3)
            {
                NS_LOG_ERROR("Invalid IP address format.");
                throw std::invalid_argument("Invalid IP address format");
            }
            res[i] = static_cast<char>(std::stoi(str.substr(start, end - start)));
            start = end + 1;
        }
        return res;
    }
    catch (const std::exception& e)
    {
        NS_LOG_ERROR("Exception in IpStrToBytes: " << e.what());
        return "";
    }
}

} // namespace ns3
