#pragma once

#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <string>
#include <deque>
#include <mutex>

class ChatServer;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	explicit ClientSession(boost::asio::ip::tcp::socket socket, ChatServer* chatServer);

	void Start();
	void Send(const std::string& message);

	std::string GetNickname() const;
	bool HasNickname() const;

private:
	void DoRead();
	void DoWrite();
	void Close();
	void HandleLine(const std::string& line);

private:
	bool IsValidNIckname(const std::string& nickname) const;
	bool IsValidMessage(const std::string& message) const;
	bool IsRateLimited();

private:
	static constexpr std::size_t kMaxReadBufferSize = 1024;
	static constexpr std::size_t kMaxLineLength = 512;
	static constexpr std::size_t kMaxNicknameLength = 16;
	static constexpr std::size_t kMinNicknameLength = 2;
	static constexpr std::size_t kMaxMessageLength = 200;
	static constexpr std::size_t kMaxWriteQueueSize = 100;
	static constexpr std::size_t kMaxMessagePerWindow = 5;
	static constexpr int kRateLimitWindowSeconds = 10;

private:
	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buffer_;
	ChatServer* server_;
	std::deque<std::string> writeQueue_;
	std::string nickname_;
	bool hasJoined_ = false;

private:
	bool isClosed_ = false;

private:
	std::deque<std::chrono::steady_clock::time_point> messageTimestamps_;

private:
	std::mutex writeQueueMutex_;
	std::mutex closeMutex_;
};