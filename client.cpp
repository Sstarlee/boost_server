/*
 * =====================================================================================
 *
 *       Filename:  client.cpp
 *
 *    Description:  Receiving keyboard input processes received feedback and 
 *    		    sends messages to the server, asynchronous message reads 
 *    		    and writes, blocking threads, listening, establishing 
 *    		    communication, and event runs
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sstarlee
 *
 * =====================================================================================
 */
#include "chat_message.h"
#include "structHeader.h"
#include "SerilizationObject.h"

#include <boost/asio.hpp>
#include<chrono>
#include <deque>
#include <iostream>
#include <thread>
#include<memory>
#include <cstdlib>
#include <cassert>
#include<vector>
//tcp
using boost::asio::ip::tcp;

using chat_message_queue = std::deque<chat_message>;

/*
 *客户端结构
 *1.事件写入
 *2.关闭socket
 *3.解读监听
 *4.包头包体循环读取
 *5.写入事件
 *
 * */
class chat_client {
public:
  chat_client(boost::asio::io_service &io_service,
              tcp::resolver::iterator endpoint_iterator)
      : io_service_(io_service), socket_(io_service) {
    do_connect(endpoint_iterator);
  }
//首次发言
 void write(const chat_message &msg) 
 {
    io_service_.post([this, msg]() {
      bool write_in_progress = !write_msgs_.empty();
      write_msgs_.push_back(msg);
      if (!write_in_progress) 
      {
        do_write();
      }
    });
  }

//关闭通信
  void close() 
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
  //反序列化接收监听事件
  void do_connect(tcp::resolver::iterator endpoint_iterator) 
  {
    boost::asio::async_connect(
        socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator) {
          if (!ec) 
	  {
            do_read_header();
          }
        });
  }


  //包头读取
  void do_read_header() 
  {
    boost::asio::async_read
       (socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t ) 
	{
          if (!ec && read_msg_.decode_header()) 
	  {
            do_read_body();
          } 
	  else 
	  {
            socket_.close();
          }
        });
  }


  //包体读取
  void do_read_body() 
  {
    boost::asio::async_read(
        socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t)  {
          if (!ec) {
            if (read_msg_.type() == MT_ROOM_INFO) {
              SRoomInfo info;
              std::stringstream ss(
                  std::string(read_msg_.body(),
                              read_msg_.body() + read_msg_.body_length()));
              boost::archive::text_iarchive ia(ss);
              ia & info;
              std::cout << "' 客户： '";
              std::cout << info.name();
              std::cout << "' 发言： '";
              std::cout << info.information();
              std::cout << "'\n";
            }
            do_read_header();
          } else {
            socket_.close();
          }
        });
  }

  //事件写入
  void do_write() 
  {
    boost::asio::async_write
	    (socket_, boost::asio::buffer(write_msgs_.front().data(),
             write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t ) {
          if (!ec) 
	  {
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) 
	    {
              do_write();
            }
          } else 
	  {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_service &io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "注意11111 输入格式不准确2222\n";
      return 1;
    }

    boost::asio::io_service io_service;
   
   //客户指针容器
    std::vector<std::unique_ptr<chat_client>>clientGroup;

    tcp::resolver resolver(io_service);
    auto endpoint_iterator = resolver.resolve({argv[1], argv[2]});
   
    
    //插入监听
    for(int i=0;i<5;++i)
    {
	    clientGroup.emplace_back(std::make_unique<chat_client>(io_service,endpoint_iterator));

    }
   //线程
    std::thread t([&io_service]() { io_service.run(); });

    //接收键盘输入
    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1)) 
    {
      chat_message msg;
			auto type = 0;
			std::string input(line, line + std::strlen(line));
			std::string output;
			if(parseMessage2(input, &type, output)) 
			{
				msg.setMessage(type, output.data(), output.size());

				for(auto &v:clientGroup)
					v->write(msg);

				std::cout <<"给服务器发了 " << output.size() << std::endl;
			}
    }
  //关闭通信
    for(auto&v:clientGroup)
	    v->close();
    t.join();
  } 
  catch (std::exception &e) 
  {
	  std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

