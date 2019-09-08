#include <channels/utility/new_only_limiter.h>
#include <channels/buffered_channel.h>
#include <channels/transmitter.h>
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

TEST_CASE("testing with empty parameters", "[new_only_limiter]") {
	transmitter<new_only_limiter<buffered_channel<>>> transmitter;

	unsigned calls_number = 0;
	const connection connection = transmitter.get_channel().connect([&calls_number] { ++calls_number; });
	CHECK(calls_number == 0u);

	transmitter();
	CHECK(calls_number == 1u);

	transmitter();
	CHECK(calls_number == 1u);
}

TEST_CASE("testing with non-empty parameters", "[new_only_limiter]") {
	transmitter<new_only_limiter<buffered_channel<int, int>>> transmitter;

	unsigned calls_number = 0;
	const connection connection = transmitter.get_channel().connect([&calls_number](int, int) { ++calls_number; });
	CHECK(calls_number == 0u);

	transmitter(1, 2);
	CHECK(calls_number == 1u);

	transmitter(1, 2);
	CHECK(calls_number == 1u);

	transmitter(1, 3);
	CHECK(calls_number == 2u);
}

} // namespace
} // namespace test
} // namespace channels
