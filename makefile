server:main.cpp structHeader.cpp chat_message.h structHeader.h
	g++ -DBOOST_LOG_DYN_LINK  -O2 -std=c++14 -o server main.cpp structHeader.cpp  -lboost_thread  -lboost_log -lboost_log_setup  -lboost_system -lboost_serialization -lpthread
client:client.cpp structHeader.cpp chat_message.h structHeader.h
	g++ -O2 -std=c++14 -o client client.cpp structHeader.cpp -lboost_system -lboost_serialization -lpthread
