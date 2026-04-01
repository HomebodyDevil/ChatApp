#pragma once

#include <boost/asio.hpp>
#include <memory>

class ChatServer {
public:
	ChatServer(boost::asio::io_context& ioContext, unsigned short port);

private:
	void StartAccept();

private:
	boost::asio::ip::tcp::acceptor acceptor_;
};