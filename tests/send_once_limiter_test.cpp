#include <channels/utility/send_once_limiter.h>
#include <channels/channel.h>
#include <channels/transmitter.h>
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

TEST_CASE("testing default sends limit", "[send_once_limiter]") {
	transmitter<send_once_limiter<channel<>>> transmitter;

	CHECK_NOTHROW(transmitter());
	CHECK_THROWS_AS(transmitter(), transmitter_error);
}

} // namespace
} // namespace test
} // namespace channels
