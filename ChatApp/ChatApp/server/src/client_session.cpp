#pragma once

#include "../include/client_session.h"
#include "../include/chat_server.h"

#include <iostream>
#include <sstream>

using tcp = boost::asio::ip::tcp;

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket, ChatServer * chatServer)
	: socket_(std::move(socket))
	, server_(chatServer)
{ }

void ClientSession::Start()
{
	DoRead();
}

void ClientSession::Send(const std::string& message)
{
	bool writeInProgress = false;

	{
		std::lock_guard<std::mutex> lock(writeQueueMutex_);
		writeInProgress = !writeQueue_.empty();
		writeQueue_.push_back(message);
	}

	if (!writeInProgress) {
		DoWrite();
	}
}

std::string ClientSession::GetNickname() const
{
	return nickname_;
}

bool ClientSession::HasNickname() const
{
	return !nickname_.empty();
}

void ClientSession::DoRead()
{
	auto self = shared_from_this();
		
	boost::asio::async_read_until(
		socket_,
		buffer_,
		'\n',
		[this, self](const boost::system::error_code& ec, std::size_t bytesTransferred) {
			if (ec) {
				if (ec == boost::asio::error::eof) {
					std::cout << "[Session] Client disconnected normally\n";
				}
				else {
					std::cerr << "[Session] Read Error " 
						<< ", code= " << ec.value()
						<< ", category= " << ec.category().name()
						<< ", message= "  << ec.message() << '\n';
				}

				Close();
				return;
			}

			std::istream input(&buffer_);
			std::string line;
			std::getline(input, line);

			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			std::cout << "[Recv - " << socket_.remote_endpoint() << "] : " << line << '\n';

			HandleLine(line);
			DoRead();
		}
	);
}

void ClientSession::DoWrite()
{
	auto self = shared_from_this();

	std::lock_guard<std::mutex> lock(writeQueueMutex_);
	if (writeQueue_.empty())
		return;

	boost::asio::async_write(
		socket_,
		boost::asio::buffer(writeQueue_.front()),
		[this, self](const boost::system::error_code& ec, size_t bytes_transferred) {
			if (ec) {
				std::cerr << "[Session] Write Error : " << ec.message() << '\n';
				Close();
				return;
			}

			bool hasMore = false;
			{
				std::lock_guard<std::mutex> lock(writeQueueMutex_);

				if (!writeQueue_.empty())
					writeQueue_.pop_front();

				hasMore = !writeQueue_.empty();
			}

			if (hasMore) {
				DoWrite();
			}
		}
	);
}

void ClientSession::Close()
{
	std::lock_guard<std::mutex> lock(closeMutex_);

	if (isClosed_) return;
	isClosed_ = true;

	if (server_ != nullptr && hasJoined_ && !nickname_.empty()) {
		server_->BroadcastSystem(nickname_ + " left");
	}

	boost::system::error_code ignoredEc;	// 에러 정보를 담을 변수.(형식/명시적으로만 사용)
	socket_.shutdown(tcp::socket::shutdown_both, ignoredEc);	// 소켓의 송-수신을 모두 중단.		
	socket_.close(ignoredEc);

	if (server_ != nullptr) {
		server_->Leave(shared_from_this());
	}
}

void ClientSession::HandleLine(const std::string& line)
{
	// LIST는 param(?)이 필요하지 않으므로, delim을 찾기 전에 수행토록 한다.
	if (line == "LIST") {
		if (server_ == nullptr) {
			Send("SYSTEM|Sever unavailable\n");
			return;
		}

		std::vector<std::string> nicknames = server_->GetNicknames();

		std::string payload = "USERS|";
		for (std::size_t i = 0; i < nicknames.size(); ++i) {
			payload += nicknames[i];
			if (i + 1 < nicknames.size())
				payload += ',';
		}

		Send(payload + '\n');
		return;
	}

	std::size_t delimeterPos = line.find('|');
	if (delimeterPos == std::string::npos) {
		Send("SYSTEM|Invalid command format\n");
		return;
	}

	std::string command = line.substr(0, delimeterPos);
	std::string payload = line.substr(delimeterPos + 1);

	if (command == "NICK") {
		if (payload.empty()) {
			Send("SYSTEM|Nickname cannot be empty\n");
			return;
		}

		auto self = shared_from_this();
		
		if (server_ != nullptr && !server_->IsNicknameAvailable(payload, self)) {
			Send("SYSTEM|Nickname already in use\n");
			return;
		}

		bool firstJoin = nickname_.empty();
		std::string oldNickname = nickname_;
		nickname_ = payload;

		if (firstJoin) {
			hasJoined_ = true;
			Send("SYSTEM|Nickname registered: " + nickname_ + '\n');
			if (server_ != nullptr) {
				server_->BroadcastSystem(nickname_ + " joined");
			}
		}
		else {
			Send("SYSTEM|Nickname change to: " + nickname_ + '\n');

			if (server_ != nullptr) {
				server_->BroadcastSystem(oldNickname + " changed nickname to " + nickname_);
			}
		}

		return;
	}

	if (command == "MSG") {
		if (nickname_.empty()) {
			Send("SYSTEM|Register nickname first using \"NICK|YourName\"\n");
			return;
		}

		if (payload.empty()) {
			Send("SYSTEM|Message cannot be empty\n");
			return;
		}

		if (server_ != nullptr) {
			server_->BroadcastChat(shared_from_this(), nickname_, payload);
		}

		return;
	}

	Send("SYSTEM|Unknown command\n");
}
