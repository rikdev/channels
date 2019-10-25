#include <channels/utility/connection_manager.h>
#include <channels/channel.h>
#include <channels/transmitter.h>
#include "tools/executor.h"
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

using channels::test::tools::executor;

TEST_CASE("Testing class connection_manager", "[connection_manager]") {
	using channel_type = channel<>;
	transmitter<channel_type> transmitter;
	const channel_type& channel = transmitter.get_channel();
	connection_manager manager;

	SECTION("testing connect method") {
		SECTION("without executor") {
			unsigned calls_number = 0;
			connection& c = manager.connect(channel, [&calls_number] { ++calls_number; });

			transmitter();

			CHECK(c.is_connected());
			CHECK(calls_number == 1u);
		}
		SECTION("without executor") {
			executor executor;
			unsigned calls_number = 0;
			connection& c = manager.connect(channel, &executor, [&calls_number] { ++calls_number; });

			transmitter();
			executor.run_all_tasks();

			CHECK(c.is_connected());
			CHECK(calls_number == 1u);
		}

	}
	SECTION("testing release method") {
		unsigned calls_number1 = 0;
		manager.connect(channel, [&calls_number1] { ++calls_number1; });

		executor executor;
		unsigned calls_number2 = 0;
		manager.connect(channel, &executor, [&calls_number2] { ++calls_number2; });

		manager.release();

		transmitter();
		executor.run_all_tasks();

		CHECK(calls_number1 == 0u);
		CHECK(calls_number2 == 0u);
	}
}

} // namespace
} // namespace test
} // namespace channels
