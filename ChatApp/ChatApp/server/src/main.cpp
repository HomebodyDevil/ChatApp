#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include "../include/chat_server.h"

int main() {
	try {
		constexpr unsigned short port = 9000;

		boost::asio::io_context ioContext;
		auto workGuard = boost::asio::make_work_guard(ioContext);	// 꺼지지 않게.

		ChatServer chatServer(ioContext, port);

		std::cout << "[Server] Listening on port : " << port << '\n';
		ioContext.run();
	}
	catch (std::exception& e) {
		std::cerr << "[Error] " << e.what() << '\n';
		std::cin.get();
		return 1;
	}
}