#include <channels/utility/sync_connection_manager.h>
#include <channels/channel.h>
#include <channels/transmitter.h>
#include "tools/executor.h"
#include "tools/thread_helpers.h"
#include <catch2/catch.hpp>
#include <atomic>
#include <chrono>
#include <thread>

namespace channels {
namespace test {
namespace {

using channels::test::tools::async_executor;
using channels::test::tools::executor;
using channels::test::tools::joining_thread;

TEST_CASE("Testing class sync_connection_manager", "[sync_connection_manager]") {
	using channel_type = channel<>;
	transmitter<channel_type> transmitter;
	const channel_type& channel = transmitter.get_channel();
	sync_connection_manager connection_manager;

	SECTION("Testing in single-thread environment") {
		unsigned calls_number1 = 0;
		connection_manager.connect(channel, [&calls_number1] { ++calls_number1; });

		executor executor;
		unsigned calls_number2 = 0;
		connection_manager.connect(channel, &executor, [&calls_number2] { ++calls_number2; });

		SECTION("emitting before sync_release") {
			transmitter.send();
			executor.run_all_tasks();

			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("emitting after sync_release") {
			connection_manager.sync_release();
			transmitter.send();
			executor.run_all_tasks();

			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 0u);
		}

		connection_manager.sync_release();
	}
	SECTION("Testing in multi-thread environment") {
		using namespace std::chrono_literals;

		async_executor executor;
		connection_manager.connect(channel, &executor, executor.make_synchronizable_callback([] {}));
		connection_manager.connect(channel, &executor, executor.make_synchronizable_callback([] {}));

		transmitter.send();

		std::atomic<bool> worker_release_started{false};
		std::atomic<bool> worker_release_finished{false};
		joining_thread worker_release_thread{
			[&worker_release_started, &worker_release_finished, &connection_manager] {
				worker_release_started = true;
				connection_manager.sync_release();
				worker_release_finished = true;
			}};

		while (!worker_release_started)
			std::this_thread::yield();
		std::this_thread::sleep_for(20ms);
		CHECK_FALSE(worker_release_finished);

		executor.resume_callbacks();
	}
}

} // namespace
} // namespace test
} // namespace channels
