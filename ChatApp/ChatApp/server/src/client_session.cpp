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
					std::cerr << "[Session] Read Error " << ec.message() << '\n';
				}

				Close();
				return;
			}

			std::istream input(&buffer_);
			std::string line;
			std::getline(input, line);

			std::cout << "[Recv - " << socket_.remote_endpoint() << "] : " << line << '\n';

			if (server_ != nullptr) {
				server_->Broadcast(shared_from_this(), line);
			}


			DoRead();
		}
	);
}

void ClientSession::DoWrite()
{
	boost::asio::async_write(
		socket_,
		boost::asio::buffer(writeQueue_.front()),
		[this](const boost::system::error_code& ec, size_t bytes_transferred) {
			if (ec) {
				std::cerr << "[Session] Write Error : " << ec.message() << '\n';
				Close();
				return;
			}

			{
				std::lock_guard<std::mutex> lock(writeQueueMutex_);
				writeQueue_.pop_front();

				if (!writeQueue_.empty()) {
					DoWrite();
				}
			}
		}
	);
}

void ClientSession::Close()
{
	boost::system::error_code ignoredEc;	// 에러 정보를 담을 변수.(형식/명시적으로만 사용)
	socket_.shutdown(tcp::socket::shutdown_both, ignoredEc);	// 소켓의 송-수신을 모두 중단.		
	socket_.close(ignoredEc);

	if (server_ != nullptr) {
		server_->Leave(shared_from_this());
	}
}