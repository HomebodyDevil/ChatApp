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

					auto session = std::make_shared<ClientSession>(std::move(socket));
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
