#include <channels/utility/executors.h>
#include "tools/executor.h"
#include <catch2/catch.hpp>
#include <memory>

namespace channels {
namespace utility {
namespace test {
namespace {

using channels::test::tools::executor;

TEST_CASE("Testing inline_executor class", "[inline_executor]") {
	const inline_executor testing_executor;
	unsigned calls_number = 0;
	execute(testing_executor, [&calls_number] { ++calls_number; });

	CHECK(calls_number == 1);
}

TEST_CASE("Testing tracking_executor class", "[tracking_executor]") {
	SECTION("empty tracked object") {
		const std::weak_ptr<void> tracked_object;

		SECTION("without user executor") {
			auto testing_executor = channels::utility::make_tracking_executor(tracked_object);
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });

			CHECK(calls_number == 0);
		}
		SECTION("with user executor") {
			executor user_executor;
			auto testing_executor = channels::utility::make_tracking_executor(tracked_object, &user_executor);
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });

			user_executor.run_all_tasks();
			CHECK(calls_number == 0);
		}
	}
	SECTION("non-empty tracked object") {
		const auto tracker = std::make_shared<int>(0);
		const std::weak_ptr<void> tracked_object = tracker;

		SECTION("without user executor") {
			auto testing_executor = channels::utility::make_tracking_executor(tracked_object);
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });

			CHECK(calls_number == 1);
		}
		SECTION("with user executor") {
			executor user_executor;
			auto testing_executor = channels::utility::make_tracking_executor(tracked_object, &user_executor);
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });
			CHECK(calls_number == 0);

			user_executor.run_all_tasks();
			CHECK(calls_number == 1);
		}
	}
	SECTION("non-lockable tracked object") {
		struct tracket_object_type {
			bool expired() const noexcept { return false; }
			bool lock() const noexcept { return false; }
		};

		SECTION("without user executor") {
			auto testing_executor = channels::utility::make_tracking_executor(tracket_object_type{});
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });

			CHECK(calls_number == 0);
		}
		SECTION("with user executor") {
			executor user_executor;
			auto testing_executor = channels::utility::make_tracking_executor(tracket_object_type{}, &user_executor);
			unsigned calls_number = 0;
			execute(testing_executor, [&calls_number] { ++calls_number; });

			user_executor.run_all_tasks();
			CHECK(calls_number == 0);
		}
	}
}

} // namespace
} // namespace test
} // namespace utility
} // namespace channels
