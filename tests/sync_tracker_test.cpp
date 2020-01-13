#include <channels/utility/sync_tracker.h>
#include <catch2/catch.hpp>

namespace channels {
namespace test {
namespace {

TEST_CASE("sync_tracker::tracked_object was created by default constructor", "[sync_tracker]") {
	const sync_tracker::tracked_object tracked_object;

	CHECK_FALSE(tracked_object.lock());
	CHECK(tracked_object.expired());
}

TEST_CASE("sync_tracker::tracked_object was created by sync_tracker", "[sync_tracker]") {
	sync_tracker tracker;
	const sync_tracker::tracked_object tracked_object = tracker.get_tracked_object();

	SECTION("unreleased tracker") {
		CHECK(tracked_object.lock());
		CHECK_FALSE(tracked_object.expired());
	}
	SECTION("released tracker") {
		tracker.sync_release();

		CHECK_FALSE(tracked_object.lock());
		CHECK(tracked_object.expired());
	}

	tracker.sync_release();
}

TEST_CASE("sync_tracker was move assigned", "[sync_tracker]") {
	sync_tracker tracker;
	const sync_tracker::tracked_object tracked_object = tracker.get_tracked_object();
	tracker = sync_tracker{};

	CHECK_FALSE(tracked_object.lock());
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

TEST_CASE("using sync_tracker by function execute", "[sync_tracker][execute]") {
	unsigned calls_number = 0;
	auto task = [&calls_number] { ++calls_number; };

	SECTION("unreleased tracker") {
		sync_tracker tracker;
		execute(tracker.get_tracked_object(), task);
		CHECK(calls_number == 1);

		tracker.sync_release();
	}
	SECTION("released tracker") {
		execute(sync_tracker::tracked_object{}, task);
		CHECK(calls_number == 0);
	}
}

} // namespace
} // namespace test
} // namespace channels
