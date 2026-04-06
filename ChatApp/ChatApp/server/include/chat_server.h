#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <unordered_set>
#include <string>
#include <mutex>
#include <vector>

class ClientSession;

class ChatServer {
public:
	ChatServer(boost::asio::io_context& ioContext, unsigned short port);

	void Join(const std::shared_ptr<ClientSession>& session);
	void Leave(const std::shared_ptr<ClientSession>& session);

	void BroadcastRaw(const std::shared_ptr<ClientSession> fromSession_, const std::string& message);
	void BroadcastSystem(const std::string& message);
	void BroadcastChat(const std::shared_ptr<ClientSession> fromSession_, const std::string& nickname, const std::string& message);
	//void Broadcast(std::shared_ptr<ClientSession> session, const std::string& message);

	bool IsNicknameAvailable(const std::string& nickname, const std::shared_ptr<ClientSession>& requester) const;
	std::vector<std::string> GetNicknames() const;

private:
	void StartAccept();

private:
	boost::asio::ip::tcp::acceptor acceptor_;
	std::unordered_set<std::shared_ptr<ClientSession>> sessions_;

private:
	mutable std::mutex sessionsMutex_;
};