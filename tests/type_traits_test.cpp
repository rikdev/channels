#include <channels/channel_traits.h>
#include <channels/channel.h>
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

TEST_CASE("Testing is_applicable", "[is_applicable]") {
	SECTION("channel arguments is empty, is_applicable arguments is empty") {
			constexpr bool applicable = is_applicable_v<channel<>>;

			CHECK(applicable);
	}
	SECTION("channel arguments isn't empty (but default constructible), is_applicable arguments is empty") {
			constexpr bool applicable = is_applicable_v<channel<int>>;

			CHECK_FALSE(applicable);
	}
	SECTION("channel arguments is empty, is_applicable arguments isn't empty") {
			constexpr bool applicable = is_applicable_v<channel<>, int>;

			CHECK_FALSE(applicable);
	}
	SECTION("channel arguments is constructible from is_applicable arguments") {
			constexpr bool applicable = is_applicable_v<channel<std::string, double>, const char*, float>;

			CHECK(applicable);
	}
	SECTION("channel arguments isn't constructible from is_applicable arguments") {
			constexpr bool applicable = is_applicable_v<channel<std::string, double>, int, float>;

			CHECK_FALSE(applicable);
	}
}

} // namespace
} // namespace test
} // namespace channels
