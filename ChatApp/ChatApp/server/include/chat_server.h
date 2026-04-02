#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <unordered_set>
#include <string>
#include <mutex>

class ClientSession;

class ChatServer {
public:
	ChatServer(boost::asio::io_context& ioContext, unsigned short port);

	void Join(const std::shared_ptr<ClientSession>& session);
	void Leave(const std::shared_ptr<ClientSession>& session);
	void Broadcast(std::shared_ptr<ClientSession> session, const std::string& message);

private:
	void StartAccept();

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	std::unordered_set<std::shared_ptr<ClientSession>> sessions_;

private:
	std::mutex sessionsMutex_;
};