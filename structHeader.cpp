/*
 * =====================================================================================
 *
 *       Filename:  structHeader.cpp
 *
 *    Description:  Handles keyboard reception and defines serialization and 
 *    		    deserialization of input and output streams
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sstarlee
 *   
 *
 * =====================================================================================
 */
#include "structHeader.h"
#include "SerilizationObject.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
//重载 & 定义序列化与反序列化流
template <typename T> std::string seriliaze(const T &obj) 
{
  std::stringstream ss;
  boost::archive::text_oarchive oa(ss);
  oa & obj;
  return ss.str();
}
//处理键盘输入
bool parseMessage2(const std::string& input, int* type, std::string& outbuffer) {
  auto pos = input.find_first_of(" ");
  if (pos == std::string::npos)
    return false;
  if (pos == 0)
    return false;
	// "BindName ok" -> substr -> BindName
  auto command = input.substr(0, pos);
  if (command == "Name") {
    // we try to bind name
    std::string name = input.substr(pos + 1);
    if (name.size() > 32)
      return false;
    if (type)
      *type = MT_BIND_NAME;
    //SBindName bindInfo(std::move(name));
    outbuffer = seriliaze(SBindName(std::move(name)));
    return true;
  } else if (command == "Chat") {
    // we try to chat
    std::string chat = input.substr(pos + 1);
    if (chat.size() > 256)
      return false;
		outbuffer = seriliaze(SChatInfo(std::move(chat)));
    if (type)
      *type = MT_CHAT_INFO;
    return true;
  }
  return false;
}

