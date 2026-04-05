#pragma once

#include "../include/chat_server.h"
#include "../include/client_session.h"
#include <iostream>

using tcp = boost::asio::ip::tcp;

ChatServer::ChatServer(boost::asio::io_context& ioContext, unsigned short port)
	: acceptor_(ioContext, tcp::endpoint(tcp::v4(), port))
{
	StartAccept();
}

void ChatServer::Join(const std::shared_ptr<ClientSession>& session)
{
	std::lock_guard<std::mutex> lock(sessionsMutex_);

	sessions_.insert(session);
	std::cout << "[Server] Session joined. Current sessions : " << sessions_.size() << '\n';
}

void ChatServer::Leave(const std::shared_ptr<ClientSession>& session)
{
	std::lock_guard<std::mutex> lock(sessionsMutex_);

	std::string packet = session->GetNickname() + " left";
	BroadcastSystem(packet);

	sessions_.erase(session);
	std::cout << "[Server] Sessions left. Current sessions : " << sessions_.size() << '\n';
}

void ChatServer::BroadcastRaw(const std::shared_ptr<ClientSession> fromSession_, const std::string& message)
{
	std::lock_guard<std::mutex> lock(sessionsMutex_);

	for (const auto& session : sessions_) {
		if (fromSession_ != nullptr && session == fromSession_) continue;
		session->Send(message + '\n');
	}
}

void ChatServer::BroadcastSystem(const std::string& message)
{
	std::string packet = "SYSTEM | " + message;
	BroadcastRaw(nullptr, packet);

	std::cout << "[System] " << message << '\n';
}

void ChatServer::BroadcastChat(const std::shared_ptr<ClientSession> fromSession_, const std::string& nickname, const std::string& message)
{
	std::string packet = nickname + ": " + message;
	BroadcastRaw(fromSession_, packet);

	std::cout << "[Chat] " << nickname << ": " << message << '\n';
}

//void ChatServer::Broadcast(std::shared_ptr<ClientSession> fromSession, const std::string& message)
//{
//	std::lock_guard<std::mutex> lock(sessionsMutex_);
//
//	for (const auto& session : sessions_) {		
//		if (fromSession != session) {
//			session->Send(message + '\n');
//		}
//	}
//
//	std::cout << "[Broadcast] : " << message << '\n';
//}

void ChatServer::StartAccept()
{
	acceptor_.async_accept(
		[this](const boost::system::error_code& ec, tcp::socket socket) {
			try {
				if (!ec) {
					std::cout << "[Server] Client connected: "
						<< socket.remote_endpoint().address().to_string()
						<< " : "
						<< socket.remote_endpoint().port()
						<< '\n';

					auto session = std::make_shared<ClientSession>(std::move(socket), this);
					Join(session);
					session->Start();
				}
				else {
					std::cerr << "[Server] Accept Error " << ec.message() << '\n';
				}
			}
			catch (const std::exception& e) {
				std::cerr << "[Server] Accept Handler Error : " << e.what() << '\n';				
			}

			StartAccept();
		}
	);
}
