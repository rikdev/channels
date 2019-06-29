#include <channels/utility/no_executor_limiter.h>
#include <channels/channel.h>
#include <channels/transmitter.h>
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

TEST_CASE("Testing class no_executor_limiter", "[no_executor_limiter]") {
	SECTION("testing method is_valid") {
		using channel_type = no_executor_limiter<channel<>>;

		SECTION("for channel without shared_state") {
			const channel_type channel;

			CHECK_FALSE(channel.is_valid());
		}
		SECTION("for channel with shared_state") {
			const transmitter<channel_type> transmitter;

			CHECK(transmitter.is_valid());
		}
	}
	SECTION("testing method connect") {
		using channel_type = no_executor_limiter<channel<>>;
		transmitter<channel_type> transmitter;

		const connection connection = transmitter.connect([] {});
		CHECK(connection.is_connected());
	}
}

} // namespace
} // namespace test
} // namespace channels
