/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  The main logical parts of the boost server include io_service
 *                  and listening ports, buffers, manipulation of data, and
 *                  multithreading
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sstarlee
 *    
 * =====================================================================================
 */
#include "chat_message.h"
#include<boost/log/trivial.hpp>
#include<boost/log/core.hpp>
#include<boost/log/expressions.hpp>
#include<boost/log/utility/setup/file.hpp>
#include<chrono>
#include <boost/asio.hpp>
#include<thread>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include<vector>
#include <cstdlib>

using boost::asio::ip::tcp;//tcp连接

using chat_message_queue = std::deque<chat_message>;
//using chat_message_queue2 = std::list<chat_message>;

std::chrono::system_clock::time_point base;//时间戳


class chat_session;
using chat_session_ptr = std::shared_ptr<chat_session>;

/*聊天空间结构
 *主要有下列一些功能；
 *1.新成员加入后分发存储的消息（100条）
 *2.有人退出后删除
 *3.分发消息
 *
 * */

class chat_room 
{
public:
	chat_room(boost::asio::io_service &io_service) : m_strand(io_service){}
public:
	void join(chat_session_ptr);
	void leave(chat_session_ptr);
	void deliver(const chat_message&);
private:

	boost::asio::io_service::strand m_strand;
	std::set<chat_session_ptr> participants_;
	enum { max_recent_msgs = 100 };
	chat_message_queue recent_msgs_;
};



/*
 *消息处理结构
 *功能：
 *1.启动整个IO处理结构
 *2.空队列时第一次的写入
 *3.包的头部读取（内嵌包体读取）
 *4.包的包体读取（内嵌包头读取）
 *
 * */

class chat_session : public std::enable_shared_from_this<chat_session> 
{
public:
   chat_session(tcp::socket socket, chat_room &room)
		: socket_(std::move(socket)), room_(room) {}
  void start() 
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message &msg) 
  {
	
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
			// first
    do_write();
    }
  };

private:
  void do_read_header() 
  {
    auto self(shared_from_this());
     
    boost::asio::async_read
	    (socket_,boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t ) {
          if (!ec && read_msg_.decode_header()) {
            do_read_body();
          } 
	  else 
	  {
            std::cout << "有人离开了讨论\n";
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body() 
  {
    auto self(shared_from_this());
    
    boost::asio::async_read(
        socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
         
	[this, self](boost::system::error_code ec, std::size_t ) {
          if (!ec) 
	  {
            handleMessage();
            do_read_header();
          } 
	  else 
	  {
            room_.leave(shared_from_this());
          }
        });
  }

	template<typename T>
		T toObject() 
		{
			T obj;
			std::stringstream ss(std::string(read_msg_.body(), read_msg_.body() + read_msg_.body_length()));
			boost::archive::text_iarchive oa(ss);
			oa & obj;
			return obj;
		}

  void handleMessage() 
  {
       
	  auto n = std::chrono::system_clock::now()-base;//和基准时间相减

        
	  /*加上时间戳，停顿千分之三秒*/
	  std::cout<<"我正在这个线程："<<std::this_thread::get_id()<<" 时间:"<<
	  std::chrono::duration_cast<std::chrono::milliseconds>(n).count()<<std::endl;
	  std::this_thread::sleep_for(std::chrono::milliseconds(3));


		 /* 接受客户端名和聊天内容输入*/		
		 if (read_msg_.type() == MT_BIND_NAME) 
		 {

			 auto bindName = toObject<SBindName>();
			 m_name = bindName.bindName();
		 } else if (read_msg_.type() == MT_CHAT_INFO) 
		 {
			 auto chat = toObject<SChatInfo>();
			 m_chatInformation = chat.chatInformation();
			 auto rinfo = buildRoomInfo();
			 chat_message msg;
			 msg.setMessage(MT_ROOM_INFO, rinfo);
			 room_.deliver(msg);

		 }   }


  /*异步写入消息对象（buffer）*/
  void do_write() 
  {
	  auto self(shared_from_this());
	  boost::asio::async_write(
			  socket_, boost::asio::buffer(write_msgs_.front().data(),
				  write_msgs_.front().length()),

			  [this, self](boost::system::error_code ec, std::size_t /*length*/) {
			  if (!ec) 
			  {
			  write_msgs_.pop_front();
			  if (!write_msgs_.empty()) {
			  do_write();
			  }
			  } 
			  else 
			  {
			  room_.leave(shared_from_this());
			  }
			  });
  }	      



  	tcp::socket socket_;
 	chat_room &room_;
  	chat_message read_msg_;
  	chat_message_queue write_msgs_;
  	std::string m_name;
  	std::string m_chatInformation;
/*RoomInfo的接收*/
  std::string buildRoomInfo() const 
  {
	  SRoomInfo roomInfo( m_name,m_chatInformation);
	  std::stringstream ss;
	  boost::archive::text_oarchive oa(ss);
	  oa & roomInfo;
	  return ss.str();
  }
};

/*异步发布io 避免运行过程中几个线程run一个时间*/
void chat_room::join(chat_session_ptr participant) 
{
       m_strand.post([this,participant]{
	participants_.insert(participant);
	for (const auto& msg : recent_msgs_)
		participant->deliver(msg);
			});
}

void chat_room::leave(chat_session_ptr participant) 
{
//     异步  发布	
       	m_strand.post([this,participant]{
	participants_.erase(participant);});

}

void chat_room::deliver(const chat_message &msg) 
{

	m_strand.post([this,msg]
			{
			recent_msgs_.push_back(msg);
			while (recent_msgs_.size() > max_recent_msgs)
			recent_msgs_.pop_front();
			}
		      );

	for (auto& participant : participants_)
		participant->deliver(msg);
}


/*服务器结构
 * 事件接收（交给session去处理）
 * */
class chat_server 
{
public:
  chat_server(boost::asio::io_service &io_service,
              const tcp::endpoint &endpoint)
      : acceptor_(io_service, endpoint), socket_(io_service),room_(io_service)
  {
    do_accept();
  }

private:
  void do_accept() 
  {
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec) 
		    {
      if (!ec) 
      {
        auto session =
            std::make_shared<chat_session>(std::move(socket_), room_);
        session->start();
      }

      do_accept();
    });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};



/*日志记录*/
void init()
{	
	boost::log::add_file_log("sample.log");
/*	boost::log::core::get()->set_filter
		(
		 boost::log::trivial::severity>=boost::log::trivial::info
		);*/
}



int main(int argc, char *argv[]) 
{
  try {
	 init();
	  BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
	  BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
	  BOOST_LOG_TRIVIAL(info) << "An informational severity message";
	  BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
	  BOOST_LOG_TRIVIAL(error) << "An error severity message";
	  BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";

    if (argc < 2) 
    {
      std::cerr << "请准确输入：\n";
      return 1;
    }

    base = std::chrono::system_clock::now();//初始化时间戳
    
    boost::asio::io_service io_service;

    /*  监听事件  IP+端口号确定传输socket   */
    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i) 
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_service, endpoint);
    }

    //线程容器 io_service.run()
    std::vector<std::thread>threadGroup;
    for(int i=0;i<5;++i)
    {
	    threadGroup.emplace_back([&io_service,i]
		{
		   std::cout<<i<<"线程在 "<<std::this_thread::get_id()<<std::endl;
		   io_service.run();
		});
    }

    std::cout<<"主线程在 "<<std::this_thread::get_id()<<std::endl;
    io_service.run();
    for(auto &v:threadGroup)v.join();
  }
  //异常处理
  catch (std::exception &e) 
  {
    BOOST_LOG_TRIVIAL(error)<<"Exception"<<e.what();
    std::cerr << "Exception: " << e.what() << "\n";
  }
    BOOST_LOG_TRIVIAL(info) << "正常退出。。。。";


  return 0;
}
