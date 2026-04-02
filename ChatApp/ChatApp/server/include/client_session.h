#pragma once

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <string>
#include <deque>
#include <mutex>

class ChatServer;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	explicit ClientSession(boost::asio::ip::tcp::socket socket, ChatServer* chatServer);

	void Start();
	void Send(const std::string& message);

private:
	void DoRead();
	void DoWrite();
	void Close();

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buffer_;
	ChatServer* server_;
	std::deque<std::string> writeQueue_;

private:
	std::mutex writeQueueMutex_;
};