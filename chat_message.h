/*
 * =====================================================================================
 *
 *       Filename:  chat_message.h
 *
 *    Description:  The specific design of the message protocol and the related 
 *    		    processing after the keyboard receives the event
 *
 * 
 *
 *
 *        Version:  1.0
 *
 * 
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  Sstarlee
 * =====================================================================================
 */
#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include "structHeader.h"
#include "SerilizationObject.h"

#include <iostream>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

class chat_message {
public:
  enum { header_length = sizeof(Header) };
  enum { max_body_length = 512 };

  chat_message(){}

  const char *data() const { return data_; }

  char *data() { return data_; }

  std::size_t length() const { return header_length + m_header.bodySize; }

  const char *body() const { return data_ + header_length; }

  char *body() { return data_ + header_length; }
  int type() const { return m_header.type; }

  std::size_t body_length() const { return m_header.bodySize; }
  
 //从键盘收到事件后编写事件格式
  void setMessage(int messageType, const void *buffer, size_t bufferSize) 
  {
  	        assert(bufferSize <= max_body_length);
		m_header.bodySize = bufferSize;
		m_header.type = messageType;
		std::memcpy(body(), buffer, bufferSize);
		std::memcpy(data(), &m_header, sizeof(m_header));
  }
 void setMessage(int messageType, const std::string& buffer) 
  	{
		setMessage(messageType, buffer.data(), buffer.size());
	}

 //撰写头部 type body_size
  bool decode_header() 
  {
    std::memcpy(&m_header, data(), header_length);
    if (m_header.bodySize > max_body_length) 
    {
      std::cout << "body size " << m_header.bodySize << " " << m_header.type
                << std::endl;
      return false;
    }
    return true;
  }

private:
  char data_[header_length + max_body_length];
	Header m_header;
};

#endif 
