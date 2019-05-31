#include "inefficient_timer.h"
#include <utility>

inefficient_timer::inefficient_timer(std::chrono::milliseconds period, cancellation_token_type cancellation_token)
	: period_{period}
	, cancellation_token_{std::move(cancellation_token)}
	, thread_{[this] { run(); }}
{
	token_connection_ = cancellation_token_.connect([this]() noexcept { notifier_.notify_one(); });
}

inefficient_timer::~inefficient_timer()
{
	token_connection_.disconnect();
	thread_.join();
}

inefficient_timer::channel_type inefficient_timer::get_channel() const noexcept
{
	return transmitter_;
}

void inefficient_timer::run()
{
	for (;;) {
		std::unique_lock<decltype(mutex_)> lock{mutex_};
		if (notifier_.wait_for(lock, period_, [this] { return static_cast<bool>(cancellation_token_.get_value()); }))
			return;

		lock.unlock();
		transmitter_(++tick_count_);
	}
}
