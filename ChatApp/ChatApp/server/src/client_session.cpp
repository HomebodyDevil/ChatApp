#pragma once

#include "../include/client_session.h"
#include <iostream>
#include <sstream>

using tcp = boost::asio::ip::tcp;

ClientSession::ClientSession(boost::asio::ip::tcp::socket socket)
	: socket_(std::move(socket))
{ }

void ClientSession::Start()
{
	DoRead();
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
				return;
			}

			std::istream input(&buffer_);
			std::string line;
			std::getline(input, line);

			std::cout << "[Recv] " << line << '\n';

			DoRead();
		}
	);
}
