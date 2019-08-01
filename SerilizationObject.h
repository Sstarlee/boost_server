/*
 * =====================================================================================
 *
 *       Filename:  SerilizationObject.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sstarlee 
 *   
 * =====================================================================================
 */
#ifndef FND_SERI_H
#define FND_SERI_H
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
class SBindName {
private:
  friend class boost::serialization::access;
  //当类存档对应于输出存档时&运算符的定义类似于<<。
  //同样，当类存档时是输入存档的一种类型，&操作符的定义类似于>>。
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) 
  {
    ar & m_bindName;
		//ar << m_bindName;
		//ar >> m_bindName;
  }
  std::string m_bindName;

public:
  SBindName(std::string name) : m_bindName(std::move(name)) {}
	SBindName() {}
  const std::string &bindName() const { return m_bindName; }
};

class SChatInfo 
{
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) 
  {
    ar &m_chatInformation;
  }
  std::string m_chatInformation;

public:
  SChatInfo(std::string info) : m_chatInformation(std::move(info)) {}
	SChatInfo() {}
  const std::string &chatInformation() const { return m_chatInformation; }
};

class SRoomInfo 
{
public:
	SRoomInfo() {}
  SRoomInfo(std::string name, std::string info)
      : m_bind(std::move(name)), m_chat(std::move(info)) {}
	const std::string& name() const { return m_bind.bindName();}
	const std::string& information() const { return m_chat.chatInformation();}
private:

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive &ar, const unsigned int version) 
  {
    ar &m_bind;
		ar & m_chat;
  }
	SBindName m_bind;
	SChatInfo m_chat;

};

#endif

