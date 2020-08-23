#include "cancellation_token.h"
#include "inefficient_timer.h"
#include "one_thread_executor.h"
#include <channels/transmitter.h>
#include <channels/utility/send_once_limiter.h>
#include <iostream>

int main()
{
	using namespace std::chrono_literals;

	channels::transmitter<channels::utility::send_once_limiter<cancellation_token_type>> cancellation_token_source;

	one_thread_executor executor{cancellation_token_source.get_channel()};

	inefficient_timer timer{1s, cancellation_token_source.get_channel()};
	const inefficient_timer::channel_type& timer_channel = timer.get_channel();
	const channels::connection c1 = timer_channel.connect(
		&executor,
		[](const unsigned long tick_count) { std::cout << "Connection 1. Tick count: " << tick_count << std::endl; });
	const channels::connection c2 = timer_channel.connect(
		[](const unsigned long tick_count) { std::cout << "Connection 2. Tick count: " << tick_count << std::endl; });

	std::cout << "Press enter to exit\n";
	std::cin.get();

	cancellation_token_source.send();
}
