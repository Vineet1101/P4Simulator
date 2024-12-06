/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/format-utils.h"
#include "ns3/log.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <iostream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P4FormatUtils");

char *
IntToStr (int num)
{
  NS_LOG_FUNCTION (num);

  try
    {
      std::string result = std::to_string (num);
      char *cstr = new char[result.length () + 1];
      std::strcpy (cstr, result.c_str ());

      return cstr;
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Exception in IntToStr: " << e.what ());
      return nullptr;
    }
}

unsigned int
StrToInt (const std::string &str)
{
  NS_LOG_FUNCTION (str);
  unsigned int res = 0;
  try
    {
      if (str.find ("0x") == 0)
        { // Hexadecimal
          for (size_t i = 2; i < str.size (); i++)
            {
              char c = str[i];
              if (isdigit (c))
                {
                  res = res * 16 + (c - '0');
                }
              else if (isxdigit (c))
                {
                  res = res * 16 + (tolower (c) - 'a' + 10);
                }
              else
                {
                  NS_LOG_ERROR ("Invalid character in hex string: " << c);
                  throw std::invalid_argument ("Invalid hex string");
                }
            }
        }
      else if (str.find ("0b") == 0)
        { // Binary
          for (size_t i = 2; i < str.size (); i++)
            {
              char c = str[i];
              if (c == '0' || c == '1')
                {
                  res = res * 2 + (c - '0');
                }
              else
                {
                  NS_LOG_ERROR ("Invalid character in binary string: " << c);
                  throw std::invalid_argument ("Invalid binary string");
                }
            }
        }
      else
        { // Decimal
          for (size_t i = 0; i < str.size (); i++)
            {
              char c = str[i];
              if (isdigit (c))
                {
                  res = res * 10 + (c - '0');
                }
              else
                {
                  NS_LOG_ERROR ("Invalid character in decimal string: " << c);
                  throw std::invalid_argument ("Invalid decimal string");
                }
            }
        }
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Exception in StrToInt: " << e.what ());
      res = 0;
    }
  return res;
}

int
HexCharToInt (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  // 如果字符不是有效的十六进制字符，抛出异常
  throw std::invalid_argument ("Invalid hex character");
}

std::string
HexStrToBytes (const std::string &str)
{
  NS_LOG_FUNCTION (str);
  try
    {
      // 检查并去掉前缀 "0x"（如果存在）
      std::string hexStr = (str.find ("0x") == 0) ? str.substr (2) : str;
      NS_LOG_INFO ("Processed hex string: " << hexStr);

      // 验证长度必须为偶数
      if (hexStr.size () % 2 != 0)
        {
          NS_LOG_ERROR ("Hex string length must be even.");
          throw std::invalid_argument ("Hex string length must be even");
        }

      // 验证输入字符串是否只包含有效的十六进制字符
      for (char c : hexStr)
        {
          if (!std::isxdigit (c))
            {
              NS_LOG_ERROR ("Invalid character in hex string: " << c);
              throw std::invalid_argument ("Invalid character in hex string");
            }
        }

      std::string res;
      res.resize (hexStr.size () / 2);
      for (size_t i = 0, j = 0; i < hexStr.size (); i += 2, j++)
        {
          res[j] = HexCharToInt (hexStr[i]) * 16 + HexCharToInt (hexStr[i + 1]);
          NS_LOG_INFO ("Processed hex string: " << j << " item with " << res[j]);
        }
      return res;
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Exception in HexStrToBytes: " << e.what ());
      return "";
    }
}

std::string
Uint32IpToHex (unsigned int ip)
{
  // Create a stringstream to format the hexadecimal string
  std::ostringstream stream;

  // Add the "0x" prefix
  stream << "0x";

  // Format the number as a zero-padded 8-character hex
  stream << std::hex << std::setw (8) << std::setfill ('0') << ip;

  // Return the resulting string
  return stream.str ();
}

std::string
HexStrToBytes (const std::string &str, unsigned int bitWidth)
{
  // Ensure bitWidth is a multiple of 8
  if (bitWidth % 8 != 0)
    {
      throw std::invalid_argument ("bitWidth must be a multiple of 8.");
    }

  // Determine the maximum number of bytes based on bitWidth
  unsigned int maxBytes = bitWidth / 8;

  // Remove "0x" prefix if present
  std::string hexStr = str;
  if (hexStr.rfind ("0x", 0) == 0)
    { // Check if it starts with "0x"
      hexStr = hexStr.substr (2); // Remove the "0x"
    }

  // Ensure the string length is even
  if (hexStr.length () % 2 != 0)
    {
      hexStr = "0" + hexStr; // Pad with a leading zero if necessary
    }

  // Convert hex string to bytes
  std::string result;
  result.reserve (maxBytes); // Pre-allocate space for the result
  unsigned int byteCount = 0;

  for (size_t i = 0; i < hexStr.length () && byteCount < maxBytes; i += 2)
    {
      // Check if the characters are valid hexadecimal digits
      if (!std::isxdigit (hexStr[i]) || !std::isxdigit (hexStr[i + 1]))
        {
          throw std::invalid_argument ("Invalid hexadecimal character in input string.");
        }

      // Convert each pair of hex digits to a byte
      std::stringstream ss;
      ss << std::hex << hexStr.substr (i, 2);
      unsigned int byte;
      ss >> byte;
      result.push_back (static_cast<char> (byte));

      ++byteCount;
    }

  return result;
}

std::string
UintToString (unsigned int num)
{
  NS_LOG_FUNCTION (num);
  std::ostringstream oss;
  oss << num;
  return oss.str ();
}

// std::string
// Uint32ipToHex (unsigned int ip)
// {
//   NS_LOG_FUNCTION (ip);
//   std::ostringstream oss;
//   oss << "0x" << std::hex << std::setw (8) << std::setfill ('0') << ip;
//   return oss.str ();
// }

double
StrToDouble (const std::string &str)
{
  NS_LOG_FUNCTION (str);
  try
    {
      return std::stod (str);
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Exception in StrToDouble: " << e.what ());
      return 0.0;
    }
}

std::string
IpStrToBytes (const std::string &str)
{
  NS_LOG_FUNCTION (str);
  try
    {
      std::string res (4, '\0');
      size_t start = 0, end = 0;
      for (int i = 0; i < 4; i++)
        {
          end = str.find ('.', start);
          if (end == std::string::npos && i != 3)
            {
              NS_LOG_ERROR ("Invalid IP address format.");
              throw std::invalid_argument ("Invalid IP address format");
            }
          res[i] = static_cast<char> (std::stoi (str.substr (start, end - start)));
          start = end + 1;
        }
      return res;
    }
  catch (const std::exception &e)
    {
      NS_LOG_ERROR ("Exception in IpStrToBytes: " << e.what ());
      return "";
    }
}

std::string
IpStrToBytes (const std::string &str, unsigned int bitWidth)
{
  std::string res;
  res.resize (ceil (double (bitWidth) / 8));
  std::string full_ip_res = IpStrToBytes (str);
  for (size_t i = 0; i < res.size (); i++)
    {
      res[i] = full_ip_res[i];
    }
  return res;
}

std::string
IntToBytes (const std::string &inputStr, int bitWidth)
{
  // transfer from Python <runtime_CLI.py> by handwork

  std::vector<unsigned int> byte_array;
  std::string res;
  unsigned int input_num = std::stoi (inputStr); // assume decimal 10

  while (input_num > 0)
    {
      byte_array.push_back (input_num % 256);
      input_num = input_num / 256;
      bitWidth--;
    }
  if (bitWidth < 0)
    {
      std::cout << "UIn_BadParamError: too large parameter!" << std::endl;
    }
  while (bitWidth > 0)
    {
      byte_array.push_back (0);
      bitWidth--;
    }
  std::reverse (byte_array.begin (), byte_array.end ());

  // copy to the res.
  res.resize (byte_array.size ());
  for (int i = 0; i < int (byte_array.size ()); i++)
    {
      res[i] = byte_array[i];
    }
  return res;
}

std::string
ParseParam (std::string &input_str, unsigned int bitwidth)
{
  if (bitwidth == 32)
    {

      return HexStrToBytes (input_str);
    }
  else if (bitwidth == 48)
    {
      // return macAddr_to_bytes(input_str);
    }
  else if (bitwidth == 128)
    {
      // return ipv6Addr_to_bytes(input_str);
    }

  int bw = (bitwidth + 7) / 8;
  return IntToBytes (input_str, bw);
}

} // namespace ns3
