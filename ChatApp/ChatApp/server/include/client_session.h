#pragma once

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <string>

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	explicit ClientSession(boost::asio::ip::tcp::socket socket);

	void Start();

private:
	void DoRead();

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buffer_;
};