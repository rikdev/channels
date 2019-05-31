#include <channels/utility/sync_tracker.h>
#include <catch2/catch.hpp>

namespace channels {
namespace utility {
namespace test {
namespace {

TEST_CASE("sync_tracker::tracked_object was created by default constructor", "[sync_tracker]") {
	const sync_tracker::tracked_object tracked_object;

	CHECK_FALSE(tracked_object.lock());
}

TEST_CASE("sync_tracker::tracked_object was created by sync_tracker", "[sync_tracker]") {
	sync_tracker tracker;
	const sync_tracker::tracked_object tracked_object = tracker.get_tracked_object();

	SECTION("unreleased tracker") {
		CHECK(tracked_object.lock());
	}
	SECTION("released tracker") {
		tracker.sync_release();

		CHECK_FALSE(tracked_object.lock());
	}

	tracker.sync_release();
}

TEST_CASE("sync_tracker::is_valid called for released sync_tracker", "[sync_tracker]") {
	sync_tracker tracker;
	CHECK(tracker.is_valid());

	tracker.sync_release();
	CHECK_FALSE(tracker.is_valid());
}

TEST_CASE("sync_tracker::get_tracked_object called for released sync_tracker", "[sync_tracker]") {
	sync_tracker tracker;
	CHECK_NOTHROW(tracker.get_tracked_object());

	tracker.sync_release();
	CHECK_THROWS_AS(tracker.get_tracked_object(), tracker_error);
}

TEST_CASE("sync_tracker::lock_all called for released sync_tracker", "[sync_tracker]") {
	sync_tracker tracker;
	CHECK_NOTHROW(tracker.lock_all());

	tracker.sync_release();
	CHECK_THROWS_AS(tracker.lock_all(), tracker_error);
}

} // namespace
} // namespace test
} // namespace utility
} // namespace channels
