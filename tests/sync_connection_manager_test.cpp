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
namespace utility {
namespace test {
namespace {

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
			transmitter();
			executor.run_all_tasks();

			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("emitting after sync_release") {
			connection_manager.sync_release();
			transmitter();
			executor.run_all_tasks();

			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 0u);
		}

		connection_manager.sync_release();
	}
	SECTION("Testing in multi-thread environment") {
		using namespace std::chrono_literals;

		std::atomic<unsigned> workers_call_number{0};
		std::atomic<bool> exit_workers{false};
		const auto worker = [&workers_call_number, &exit_workers] {
			workers_call_number++;
			while (!exit_workers)
				std::this_thread::yield();
		};

		executor executor1;
		connection_manager.connect(channel, &executor1, worker);
		executor executor2;
		connection_manager.connect(channel, &executor2, worker);

		transmitter();
		joining_thread worker1_thread{[&executor1] { executor1.run_all_tasks(); }};
		joining_thread worker2_thread{[&executor2] { executor2.run_all_tasks(); }};

		while (workers_call_number < 2u)
			std::this_thread::yield();

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

		exit_workers = true;
	}
}

} // namespace
} // namespace test
} // namespace utility
} // namespace channels
