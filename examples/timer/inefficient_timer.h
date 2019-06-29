#pragma once
#include "cancellation_token.h"
#include <channels/channel.h>
#include <channels/transmitter.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class inefficient_timer {
public:
	using channel_type = channels::channel<unsigned long>;

	inefficient_timer(std::chrono::milliseconds period, cancellation_token_type cancellation_token);
	~inefficient_timer();

	const channel_type& get_channel() const noexcept;

private:
	void run();

	std::chrono::milliseconds period_;
	cancellation_token_type cancellation_token_;
	channels::connection token_connection_;
	channels::transmitter<channel_type> transmitter_;
	unsigned long tick_count_ = 0;
	std::mutex mutex_;
	std::condition_variable notifier_;
	std::thread thread_;
};

