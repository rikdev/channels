#include <channels/utility/callback.h>
#include <catch2/catch.hpp>
#include <memory>
#include <stdexcept>

namespace channels {
namespace utility {
namespace test {
namespace {

TEST_CASE("Tracking callback was created and called with different tracking objects", "[tracking_callback]") {
	SECTION("empty tracked object") {
		const std::weak_ptr<void> tracked_object;

		SECTION("callback return void result") {
			unsigned calls_number = 0;
			const auto tracking_callback = make_tracking_callback(tracked_object, [&calls_number] { ++calls_number; });
			tracking_callback();

			CHECK(calls_number == 0);
		}
		SECTION("callback return non-void result") {
			const auto tracking_callback = make_tracking_callback(tracked_object, [](int arg) { return arg; });
			const int return_value = tracking_callback(1);

			CHECK(return_value == 0);
		}
		SECTION("callback throw exception") {
			const auto tracking_callback = make_tracking_callback(
				tracked_object, [] { throw std::runtime_error{"Callback error"}; });

			CHECK_NOTHROW(tracking_callback());
		}
	}
	SECTION("non-empty tracked object") {
		const auto tracker = std::make_shared<int>(0);
		const std::weak_ptr<void> tracked_object = tracker;

		SECTION("callback return void result") {
			unsigned calls_number = 0;
			const auto tracking_callback = make_tracking_callback(tracked_object, [&calls_number] { ++calls_number; });
			tracking_callback();

			CHECK(calls_number == 1);
		}
		SECTION("callback return non-void result") {
			const auto tracking_callback = make_tracking_callback(tracked_object, [](int arg) { return arg; });
			const int return_value = tracking_callback(1);

			CHECK(return_value == 1);
		}
		SECTION("callback throw exception") {
			const auto tracking_callback = make_tracking_callback(
				tracked_object, [] { throw std::runtime_error{"Callback error"}; });

			CHECK_THROWS_AS(tracking_callback(), std::runtime_error);
		}
	}
}

} // namespace
} // namespace test
} // namespace utility
} // namespace channels
