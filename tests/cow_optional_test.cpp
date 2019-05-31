#include <channels/detail/cow_optional.h>
#include "tools/relation_only.h"
#include "tools/tracker.h"
#include <catch2/catch.hpp>
#include <array>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
#include <utility>

namespace channels {
namespace detail {
namespace test {
namespace {

// # tools
// ## tracker
using channels::test::tools::tracker;
using channels::test::tools::derived_tracker;
using channels::test::tools::implicit_tracker_constructible_struct;
using channels::test::tools::explicit_tracker_constructible_struct;

// ## relation_only
using channels::test::tools::relation_only_int;

// # tests
using use_shared_storage_type = std::false_type;
using use_inline_storage_type = std::true_type;

#if __cpp_lib_optional
#define STORAGE_TYPES use_shared_storage_type, use_inline_storage_type
#else
#define STORAGE_TYPES use_shared_storage_type
#endif

TEMPLATE_TEST_CASE("Testing class cow_optional", "[cow_optional]", STORAGE_TYPES) {
	constexpr bool use_inline_storage = TestType::value;

	// ## class cow_optional methods
	// ### constructors

	SECTION("creating using default constructor") {
		const cow_optional<tracker, use_inline_storage> v;

		CHECK_FALSE(v);
	}
	SECTION("creating using nullopt") {
		const cow_optional<tracker, use_inline_storage> v(nullopt);

		CHECK_FALSE(v);
	}
	SECTION("creating using emplace constructor without arguments") {
		const cow_optional<tracker, use_inline_storage> v{in_place};

		REQUIRE(v);
		CHECK(v->get_generation() == 0u);
	}
	SECTION("creating using emplace constructor with one argument") {
		const cow_optional<tracker, use_inline_storage> v{in_place, 707};

		REQUIRE(v);
		CHECK(v->get_value() == 707);
		CHECK(v->get_generation() == 0u);
	}
	SECTION("creating using emplace constructor with two arguments") {
		struct test_struct {
			test_struct(const long long v1, const int v2)
				: p1{v1}
				, p2{v2}
			{}

			long long p1;
			int p2;
		};

		const cow_optional<test_struct, use_inline_storage> v{in_place, -1, 1};

		REQUIRE(v);
		CHECK(v->p1 == -1);
		CHECK(v->p2 == 1);
	}
	SECTION("creating using emplace constructor with initializer list") {
		const cow_optional<std::vector<int>, use_inline_storage> v{in_place, {1, 2, 3}};

		REQUIRE(v);
		CHECK(*v == std::vector<int>{1, 2, 3});
	}
	SECTION("creating by value") {
		const cow_optional<int, use_inline_storage> v{51};

		REQUIRE(v);
		CHECK(*v == 51);
	}
	SECTION("creating using implicit convertible value") {
		const auto f = [](const cow_optional<implicit_tracker_constructible_struct, use_inline_storage>& v) {
			CHECK(v);
		};
		f(tracker{});
	}
	SECTION("creating using explicit convertible value") {
		const cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v{tracker{}};

		CHECK(v);
	}
	SECTION("creating using copy constructor") {
		const cow_optional<tracker, use_inline_storage> v1{in_place, 9};
		const cow_optional<tracker, use_inline_storage> v2{v1};

		REQUIRE(v1);
		CHECK(v1->get_value() == 9);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 9);
		CHECK(v2->get_generation() == (use_inline_storage ? 1u : 0u));
	}
	SECTION("creating using copy constructor by empty value") {
		const cow_optional<tracker, use_inline_storage> v1;
		const cow_optional<tracker, use_inline_storage> v2{v1};

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("creating using move constructor") {
		const cow_optional<tracker, use_inline_storage> v{cow_optional<tracker, use_inline_storage>{in_place, 11}};

		REQUIRE(v);
		CHECK(v->get_value() == 11);
		CHECK(v->get_generation() == 0u);
	}
	SECTION("creating using move constructor by empty value") {
		const cow_optional<tracker, use_inline_storage> v{cow_optional<tracker, use_inline_storage>{}};

		CHECK_FALSE(v);
	}
	SECTION("creating by cow_optional for derived class") {
		const cow_optional<derived_tracker, use_inline_storage> v1{in_place, 100};

		const auto f = [generation = use_inline_storage ? 1u : 0u](const cow_optional<tracker, use_inline_storage>& v) {
			REQUIRE(v);
			CHECK(v->get_value() == 100);
			CHECK(v->get_generation() == generation);
		};
		f(v1);

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);
	}
	SECTION("creating by empty cow_optional for derived class") {
		const cow_optional<derived_tracker, use_inline_storage> v1;

		const auto f = [](const cow_optional<tracker, use_inline_storage>& v) {
			CHECK_FALSE(v);
		};
		f(v1);

		CHECK_FALSE(v1);
	}
	SECTION("creating by implicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1{in_place};
		const cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v2{v1};

		REQUIRE(v1);
		CHECK(v1->get_generation() == 0u);
		REQUIRE(v2);
		CHECK(v2->tracker_object.get_copy_generation() == 1u);
		CHECK(v2->tracker_object.get_move_generation() == 0u);
	}
	SECTION("creating by empty implicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1;
		const cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v2{v1};

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("creating by explicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1{in_place};
		const cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v2{v1};

		REQUIRE(v1);
		CHECK(v1->get_generation() == 0u);
		REQUIRE(v2);
		CHECK(v2->tracker_object.get_copy_generation() == 1u);
		CHECK(v2->tracker_object.get_move_generation() == 0u);
	}
	SECTION("creating by empty explicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1;
		const cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v2{v1};

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("creating by r-value to cow_optional for derived class") {
		const auto f = [generation = use_inline_storage ? 1u : 0u](const cow_optional<tracker, use_inline_storage>& v) {
			REQUIRE(v);
			CHECK(v->get_value() == 100);
			CHECK(v->get_copy_generation() == 0u);
			CHECK(v->get_move_generation() == generation);
		};
		f(cow_optional<derived_tracker, use_inline_storage>{in_place, 100});
	}
	SECTION("creating by r-value to empty cow_optional for derived class") {
		const auto f = [](const cow_optional<tracker, use_inline_storage>& v) {
			CHECK_FALSE(v);
		};
		f(cow_optional<derived_tracker, use_inline_storage>{});
	}
	SECTION("creating by r-value to implicit convertible cow_optional") {
		const cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v{
			cow_optional<tracker, use_inline_storage>{in_place}};

		REQUIRE(v);
		CHECK(v->tracker_object.get_generation() == 1u);
	}
	SECTION("creating by r-value to empty implicit convertible cow_optional") {
		const cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v{
			cow_optional<tracker, use_inline_storage>{}};

		CHECK_FALSE(v);
	}
	SECTION("creating by r-value to explicit convertible cow_optional") {
		const cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v{
			cow_optional<tracker, use_inline_storage>{in_place}};

		REQUIRE(v);
		CHECK(v->tracker_object.get_generation() == 1u);
	}
	SECTION("creating by r-value to empty explicit convertible cow_optional") {
		const cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v{
			cow_optional<tracker, use_inline_storage>{}};

		CHECK_FALSE(v);
	}
#if __cpp_lib_optional
	SECTION("creating by cow_optional with different storage") {
		const cow_optional<tracker, !use_inline_storage> v1{in_place};
		const cow_optional<tracker, use_inline_storage> v2{v1};

		REQUIRE(v1);
		CHECK(v1->get_generation() == 0u);
		REQUIRE(v2);
		CHECK(v2->get_generation() <= 2u);
	}
	SECTION("creating by empty cow_optional with different storage") {
		const cow_optional<tracker, !use_inline_storage> v1;
		const cow_optional<tracker, use_inline_storage> v2{v1};

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("creating by r-value to empty cow_optional with different storage") {
		const cow_optional<tracker, use_inline_storage> v{cow_optional<tracker, !use_inline_storage>{in_place}};

		REQUIRE(v);
		CHECK(v->get_generation() <= 2u);
	}
	SECTION("creating by r-value to empty cow_optional with different storage") {
		const cow_optional<tracker, use_inline_storage> v{cow_optional<tracker, !use_inline_storage>{}};

		CHECK_FALSE(v);
	}
#endif

	// ### assignments

	SECTION("assigning nullopt") {
		cow_optional<int, use_inline_storage> v{in_place, 1970};
		v = nullopt;

		CHECK_FALSE(v);
	}
	SECTION("assigning nullopt to empty cow_optional") {
		cow_optional<int, use_inline_storage> v;
		v = nullopt;

		CHECK_FALSE(v);
	}
	SECTION("assigning cow_optional (copy assignment)") {
		const cow_optional<tracker, use_inline_storage> v1{in_place, 9};
		cow_optional<tracker, use_inline_storage> v2{in_place, 10};
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 9);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 9);
		CHECK(v2->get_copy_generation() == (use_inline_storage ? 1u : 0u));
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("assigning cow_optional to empty destination (copy assignment)") {
		const cow_optional<tracker, use_inline_storage> v1{in_place, 9};
		cow_optional<tracker, use_inline_storage> v2;
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 9);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 9);
		CHECK(v2->get_copy_generation() == (use_inline_storage ? 1u : 0u));
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("moving cow_optional (move assignment)") {
		cow_optional<tracker, use_inline_storage> v{in_place, 12};
		v = cow_optional<tracker, use_inline_storage>{in_place, 11};

		REQUIRE(v);
		CHECK(v->get_value() == 11);
		CHECK(v->get_copy_generation() == 0u);
		CHECK(v->get_move_generation() == (use_inline_storage ? 1u : 0u));
	}
	SECTION("moving empty cow_optional (move assignment)") {
		cow_optional<tracker, use_inline_storage> v{in_place, 11};
		v = cow_optional<tracker, use_inline_storage>{};

		CHECK_FALSE(v);
	}
	SECTION("moving cow_optional to empty destination (move assignment)") {
		cow_optional<tracker, use_inline_storage> v;
		v = cow_optional<tracker, use_inline_storage>{in_place, 11};

		REQUIRE(v);
		CHECK(v->get_value() == 11);
		CHECK(v->get_copy_generation() == 0u);
		CHECK(v->get_move_generation() == (use_inline_storage ? 1u : 0u));
	}
	SECTION("moving empty cow_optional (move assignment)") {
		const cow_optional<tracker, use_inline_storage> v1;
		cow_optional<tracker, use_inline_storage> v2{in_place, 9};
		v2 = v1;

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("assigning value") {
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v{in_place, tracker{}};
		v = tracker{51};

		REQUIRE(v);
		CHECK(v->tracker_object.get_value() == 51);
	}
	SECTION("assigning value to empty cow_optional") {
		cow_optional<explicit_tracker_constructible_struct, use_inline_storage> v;
		v = explicit_tracker_constructible_struct{tracker{51}};

		REQUIRE(v);
		CHECK(v->tracker_object.get_value() == 51);
	}
	SECTION("assigning cow_optional for derived class") {
		const cow_optional<derived_tracker, use_inline_storage> v1{in_place, 100};
		cow_optional<tracker, use_inline_storage> v2{in_place, 101};
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 100);
		CHECK(v2->get_copy_generation() == (use_inline_storage ? 1u : 0u));
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("assigning cow_optional for derived class to empty cow_optional") {
		const cow_optional<derived_tracker, use_inline_storage> v1{in_place, 100};
		cow_optional<tracker, use_inline_storage> v2;
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 100);
		CHECK(v2->get_copy_generation() == (use_inline_storage ? 1u : 0u));
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("assigning empty cow_optional for derived class") {
		const cow_optional<derived_tracker, use_inline_storage> v1;
		cow_optional<tracker, use_inline_storage> v2{in_place, 101};
		v2 = v1;

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("assigning implicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1{in_place, 100};
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v2{tracker{101}};
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->tracker_object.get_value() == 100);
		CHECK(v2->tracker_object.get_copy_generation() == 1u);
		CHECK(v2->tracker_object.get_move_generation() == 0u);
	}
	SECTION("assigning implicit convertible cow_optional to empty cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1{in_place, 100};
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v2;
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->tracker_object.get_value() == 100);
		CHECK(v2->tracker_object.get_copy_generation() == 1u);
		CHECK(v2->tracker_object.get_move_generation() == 0u);
	}
	SECTION("assigning empty implicit convertible cow_optional") {
		const cow_optional<tracker, use_inline_storage> v1;
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v2{tracker{101}};
		v2 = v1;

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("moving cow_optional for derived class") {
		cow_optional<tracker, use_inline_storage> v{in_place, 101};
		v = cow_optional<derived_tracker, use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->get_value() == 100);
		CHECK(v->get_copy_generation() == 0u);
		CHECK(v->get_move_generation() == (use_inline_storage ? 1u : 0u));
	}
	SECTION("moving cow_optional for derived class to empty cow_optional") {
		cow_optional<tracker, use_inline_storage> v;
		v = cow_optional<derived_tracker, use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->get_value() == 100);
		CHECK(v->get_copy_generation() == 0u);
		CHECK(v->get_move_generation() == (use_inline_storage ? 1u : 0u));
	}
	SECTION("moving empty cow_optional for derived class") {
		cow_optional<tracker, use_inline_storage> v{in_place, 101};
		v = cow_optional<derived_tracker, use_inline_storage>{};

		CHECK_FALSE(v);
	}
	SECTION("moving implicit convertible cow_optional") {
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v{tracker{101}};
		v = cow_optional<tracker, use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->tracker_object.get_value() == 100);
		CHECK(v->tracker_object.get_generation() == 1u);
	}
	SECTION("moving implicit convertible cow_optional to empty cow_optional") {
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v;
		v = cow_optional<tracker, use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->tracker_object.get_value() == 100);
		CHECK(v->tracker_object.get_generation() == 1u);
	}
	SECTION("moving empty implicit convertible cow_optional") {
		cow_optional<implicit_tracker_constructible_struct, use_inline_storage> v{tracker{101}};
		v = cow_optional<tracker, use_inline_storage>{};

		CHECK_FALSE(v);
	}
#if __cpp_lib_optional
	SECTION("assigning cow_optional with different storage") {
		const cow_optional<tracker, !use_inline_storage> v1{in_place, 100};
		cow_optional<tracker, use_inline_storage> v2{tracker{101}};
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 100);
		CHECK(v2->get_copy_generation() == 1u);
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("assigning cow_optional with different storage to empty cow_optional") {
		const cow_optional<tracker, !use_inline_storage> v1{in_place, 100};
		cow_optional<tracker, use_inline_storage> v2;
		v2 = v1;

		REQUIRE(v1);
		CHECK(v1->get_value() == 100);
		CHECK(v1->get_generation() == 0u);

		REQUIRE(v2);
		CHECK(v2->get_value() == 100);
		CHECK(v2->get_copy_generation() == 1u);
		CHECK(v2->get_move_generation() == 0u);
	}
	SECTION("assigning empty cow_optional with different storage") {
		const cow_optional<tracker, !use_inline_storage> v1;
		cow_optional<tracker, use_inline_storage> v2{tracker{101}};
		v2 = v1;

		CHECK_FALSE(v1);
		CHECK_FALSE(v2);
	}
	SECTION("moving cow_optional with different storage") {
		cow_optional<tracker, use_inline_storage> v{tracker{101}};
		v = cow_optional<tracker, !use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->get_value() == 100);
		CHECK(v->get_generation() == 1u);
	}
	SECTION("moving cow_optional with different storage to empty cow_optional") {
		cow_optional<tracker, use_inline_storage> v;
		v = cow_optional<tracker, !use_inline_storage>{in_place, 100};

		REQUIRE(v);
		CHECK(v->get_value() == 100);
		CHECK(v->get_generation() == 1u);
	}
	SECTION("moving empty cow_optional with different storage") {
		cow_optional<tracker, use_inline_storage> v{tracker{101}};
		v = cow_optional<tracker, !use_inline_storage>{};

		CHECK_FALSE(v);
	}
#endif

	// ### method swap

	SECTION("swapping cow_optional objects") {
		cow_optional<int, use_inline_storage> v1 = 1;
		cow_optional<int, use_inline_storage> v2 = 2;

		v1.swap(v2);

		REQUIRE(v1);
		CHECK(*v1 == 2);
		REQUIRE(v2);
		CHECK(*v2 == 1);
	}

	// ### observers

	SECTION("calling has_value() for non-empty cow_optional object") {
		const cow_optional<int, use_inline_storage> v{0};

		CHECK(v.has_value());
	}
	SECTION("calling has_value() for empty cow_optional object") {
		const cow_optional<int, use_inline_storage> v;

		CHECK_FALSE(v.has_value());
	}
	SECTION("calling value() for non-empty cow_optional object") {
		const cow_optional<int, use_inline_storage> v{3};

		CHECK(v.value() == 3);
	}
	SECTION("calling value() for empty cow_optional object") {
		const cow_optional<int, use_inline_storage> v;

		CHECK_THROWS_AS(v.value(), bad_optional_access);
	}
	SECTION("calling value() for r-value reference to cow_optional object") {
		cow_optional<tracker, use_inline_storage> v{in_place};

		const tracker t = std::move(v).value();
		CHECK(t.get_generation() == 1u);
	}
	SECTION("calling value() for r-value empty cow_optional object") {
		cow_optional<tracker, use_inline_storage> v;

		CHECK_THROWS_AS(std::move(v).value(), bad_optional_access);
	}
	SECTION("calling value() for const r-value cow_optional object") {
		const cow_optional<tracker, use_inline_storage> v{in_place};

		const tracker t = std::move(v).value();
		CHECK(t.get_copy_generation() == 1u);
		CHECK(t.get_move_generation() == 0u);
	}
	SECTION("calling value() for const r-value empty cow_optional object") {
		const cow_optional<tracker, use_inline_storage> v;

		CHECK_THROWS_AS(std::move(v).value(), bad_optional_access);
	}
	SECTION("calling value_or_data() for cow_optional object") {
		const cow_optional<tracker, use_inline_storage> v{in_place, 3};

		const tracker t = v.value_or(tracker{1});
		CHECK(t.get_copy_generation() == 1u);
		CHECK(t.get_move_generation() == 0u);
		CHECK(t.get_value() == 3);
	}
	SECTION("calling value_or_data() for empty cow_optional object") {
		cow_optional<tracker, use_inline_storage> v;

		const tracker t = v.value_or(tracker{1});
		CHECK(t.get_value() == 1);
		CHECK(t.get_copy_generation() == 0u);
		CHECK(t.get_move_generation() == 1u);
	}
	SECTION("calling value_or_data() for r-value cow_optional object") {
		cow_optional<tracker, use_inline_storage> v{in_place, 3};

		const tracker t = std::move(v).value_or(tracker{1});
		CHECK(t.get_generation() == 1u);
		CHECK(t.get_value() == 3);
	}
	SECTION("calling value_or_data() for r-value empty cow_optional object") {
		cow_optional<tracker, use_inline_storage> v;

		const tracker t = std::move(v).value_or(tracker{1});
		CHECK(t.get_value() == 1);
		CHECK(t.get_copy_generation() == 0u);
		CHECK(t.get_move_generation() == 1u);
	}

	// ### modifiers

	SECTION("calling reset() for non-empty cow-optional") {
		cow_optional<tracker, use_inline_storage> v{3};
		v.reset();

		CHECK_FALSE(v);
	}
	SECTION("calling reset() for empty cow-optional") {
		cow_optional<tracker, use_inline_storage> v;
		v.reset();

		CHECK_FALSE(v);
	}

	// ## non-member functions
	// ### relational operations
	// #### operator==

	SECTION("calling operator==(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs == rhs);
	}
	SECTION("calling operator==(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(lhs == rhs);
	}
	SECTION("calling operator==(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs == rhs);
	}
	SECTION("calling operator==(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs == rhs);
	}
	SECTION("calling operator==(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(lhs == rhs);
	}
	SECTION("calling operator==(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs == rhs);
	}

	// #### operator!=

	SECTION("calling operator!=(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs != rhs);
	}
	SECTION("calling operator!=(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(lhs != rhs);
	}
	SECTION("calling operator!=(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs != rhs);
	}
	SECTION("calling operator!=(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs != rhs);
	}
	SECTION("calling operator!=(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(lhs != rhs);
	}
	SECTION("calling operator!=(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs != rhs);
	}

	// #### operator<

	SECTION("calling operator<(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs < rhs);
	}
	SECTION("calling operator<(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(lhs < rhs);
	}
	SECTION("calling operator<(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs < rhs);
	}
	SECTION("calling operator<(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs < rhs);
	}
	SECTION("calling operator<(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(lhs < rhs);
	}
	SECTION("calling operator<(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs < rhs);
	}

	// #### operator>

	SECTION("calling operator>(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs > rhs);
	}
	SECTION("calling operator>(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(lhs > rhs);
	}
	SECTION("calling operator>(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs > rhs);
	}
	SECTION("calling operator>(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs > rhs);
	}
	SECTION("calling operator>(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(lhs > rhs);
	}
	SECTION("calling operator>(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs > rhs);
	}

	// #### operator<=

	SECTION("calling operator<=(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs <= rhs);
	}
	SECTION("calling operator<=(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(lhs <= rhs);
	}
	SECTION("calling operator<=(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(lhs <= rhs);
	}
	SECTION("calling operator<=(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs <= rhs);
	}
	SECTION("calling operator<=(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(lhs <= rhs);
	}
	SECTION("calling operator<=(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(lhs <= rhs);
	}

	// #### operator_ge

	SECTION("calling operator>=(empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs >= rhs);
	}
	SECTION("calling operator>=(empty, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(lhs >= rhs);
	}
	SECTION("calling operator>=(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs >= rhs);
	}
	SECTION("calling operator>=(non-empty, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(lhs >= rhs);
	}
	SECTION("calling operator>=(eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs >= rhs);
	}
	SECTION("calling operator>=(lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(lhs >= rhs);
	}
	SECTION("calling operator>=(gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(lhs >= rhs);
	}

	// #### operator== with nullopt

	SECTION("calling operator==(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs == nullopt);
	}
	SECTION("calling operator==(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(nullopt == rhs);
	}
	SECTION("calling operator==(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK_FALSE(lhs == nullopt);
	}
	SECTION("calling operator==(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(nullopt == rhs);
	}

	// #### operator!= with nullopt

	SECTION("calling operator!=(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs != nullopt);
	}
	SECTION("calling operator!=(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(nullopt != rhs);
	}
	SECTION("calling operator!=(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK(lhs != nullopt);
	}
	SECTION("calling operator!=(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(nullopt != rhs);
	}

	// #### operator< with nullopt

	SECTION("calling operator<(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs < nullopt);
	}
	SECTION("calling operator<(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(nullopt < rhs);
	}
	SECTION("calling operator<(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK_FALSE(lhs < nullopt);
	}
	SECTION("calling operator<(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(nullopt < rhs);
	}

	// #### operator<= with nullopt

	SECTION("calling operator<=(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs <= nullopt);
	}
	SECTION("calling operator<=(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(nullopt <= rhs);
	}
	SECTION("calling operator<=(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK_FALSE(lhs <= nullopt);
	}
	SECTION("calling operator<=(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK(nullopt <= rhs);
	}

	// #### operator> with nullopt

	SECTION("calling operator>(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs > nullopt);
	}
	SECTION("calling operator>(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(nullopt > rhs);
	}
	SECTION("calling operator>(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK(lhs > nullopt);
	}
	SECTION("calling operator>(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(nullopt > rhs);
	}

	// #### operator<= with nullopt

	SECTION("calling operator>=(empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs >= nullopt);
	}
	SECTION("calling operator>=(nullopt, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(nullopt >= rhs);
	}
	SECTION("calling operator>=(non-empty, nullopt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{0};

		CHECK(lhs >= nullopt);
	}
	SECTION("calling operator>=(nullopt, non-empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{0};

		CHECK_FALSE(nullopt >= rhs);
	}

	// #### operator== with value

	SECTION("calling operator==(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs == relation_only_int{0});
	}
	SECTION("calling operator==(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(relation_only_int{0} == rhs);
	}
	SECTION("calling operator==(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs == relation_only_int{1});
	}
	SECTION("calling operator==(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{1} == rhs);
	}
	SECTION("calling operator==(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs == relation_only_int{2});
	}
	SECTION("calling operator==(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{2} == rhs);
	}
	SECTION("calling operator==(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK_FALSE(lhs == relation_only_int{1});
	}
	SECTION("calling operator==(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(relation_only_int{1} == rhs);
	}

	// #### operator!= with value

	SECTION("calling operator!=(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs != relation_only_int{0});
	}
	SECTION("calling operator!=(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(relation_only_int{0} != rhs);
	}
	SECTION("calling operator!=(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs != relation_only_int{1});
	}
	SECTION("calling operator!=(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{1} != rhs);
	}
	SECTION("calling operator!=(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs != relation_only_int{2});
	}
	SECTION("calling operator!=(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{2} != rhs);
	}
	SECTION("calling operator!=(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK(lhs != relation_only_int{1});
	}
	SECTION("calling operator!=(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(relation_only_int{1} != rhs);
	}

	// #### operator< with value

	SECTION("calling operator<(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs < relation_only_int{0});
	}
	SECTION("calling operator<(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(relation_only_int{0} < rhs);
	}
	SECTION("calling operator<(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs < relation_only_int{1});
	}
	SECTION("calling operator<(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{1} < rhs);
	}
	SECTION("calling operator<(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs < relation_only_int{2});
	}
	SECTION("calling operator<(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{2} < rhs);
	}
	SECTION("calling operator<(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK_FALSE(lhs < relation_only_int{1});
	}
	SECTION("calling operator<(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(relation_only_int{1} < rhs);
	}

	// #### operator<= with value

	SECTION("calling operator<=(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK(lhs <= relation_only_int{0});
	}
	SECTION("calling operator<=(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK_FALSE(relation_only_int{0} <= rhs);
	}
	SECTION("calling operator<=(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs <= relation_only_int{1});
	}
	SECTION("calling operator<=(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{1} <= rhs);
	}
	SECTION("calling operator<=(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs <= relation_only_int{2});
	}
	SECTION("calling operator<=(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{2} <= rhs);
	}
	SECTION("calling operator<=(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK_FALSE(lhs <= relation_only_int{1});
	}
	SECTION("calling operator<=(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK(relation_only_int{1} <= rhs);
	}

	// #### operator> with value

	SECTION("calling operator>(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs > relation_only_int{0});
	}
	SECTION("calling operator>(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(relation_only_int{0} > rhs);
	}
	SECTION("calling operator>(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs > relation_only_int{1});
	}
	SECTION("calling operator>(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK_FALSE(relation_only_int{1} > rhs);
	}
	SECTION("calling operator>(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs > relation_only_int{2});
	}
	SECTION("calling operator>(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{2} > rhs);
	}
	SECTION("calling operator>(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK(lhs > relation_only_int{1});
	}
	SECTION("calling operator>(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(relation_only_int{1} > rhs);
	}

	// #### operator<= with value

	SECTION("calling operator>=(empty, value)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs;

		CHECK_FALSE(lhs >= relation_only_int{0});
	}
	SECTION("calling operator>=(value, empty)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs;

		CHECK(relation_only_int{0} >= rhs);
	}
	SECTION("calling operator>=(eq, value_eq)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK(lhs >= relation_only_int{1});
	}
	SECTION("calling operator>=(value_eq, eq)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{1} >= rhs);
	}
	SECTION("calling operator>=(lt, value_gt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{1};

		CHECK_FALSE(lhs >= relation_only_int{2});
	}
	SECTION("calling operator>=(value_gt, lt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{1};

		CHECK(relation_only_int{2} >= rhs);
	}
	SECTION("calling operator>=(gt, value_lt)") {
		const cow_optional<relation_only_int, use_inline_storage> lhs{2};

		CHECK(lhs >= relation_only_int{1});
	}
	SECTION("calling operator>=(value_lt, gt)") {
		const cow_optional<relation_only_int, use_inline_storage> rhs{2};

		CHECK_FALSE(relation_only_int{1} >= rhs);
	}

	// ### specialized algorithms

	SECTION("calling swap(cow_optional, cow_optional)") {
		cow_optional<tracker, use_inline_storage> v1{1};
		cow_optional<tracker, use_inline_storage> v2{2};

		swap(v1, v2);

		REQUIRE(v1);
		CHECK(v1->get_value() == 2);
		REQUIRE(v2);
		CHECK(v2->get_value() == 1);
	}
	SECTION("calling make_cow_optional(value)") {
		const cow_optional<tracker, use_inline_storage> v = make_cow_optional<tracker, use_inline_storage>(tracker{3});

		REQUIRE(v);
		CHECK(v->get_value() == 3);
	}
	SECTION("calling make_cow_optional<value_type>(value_ctor_args)") {
		struct test_struct {
			test_struct(const long long int v1, const int v2)
				: p1{v1}
				, p2{v2}
			{}

			long long int p1;
			int p2;
		};

		const cow_optional<test_struct, use_inline_storage> v =
			make_cow_optional<test_struct, use_inline_storage>(-1, 1);

		REQUIRE(v);
		CHECK(v->p1 == -1);
		CHECK(v->p2 == 1);
	}
	SECTION("calling make_cow_optional<value_type>(initializer_list)") {
		const cow_optional<std::vector<int>, use_inline_storage> v =
			make_cow_optional<std::vector<int>, use_inline_storage>({1, 2, 3});

		REQUIRE(v);
		CHECK(*v == std::vector<int>{1, 2, 3});
	}
	SECTION("calling hash(nullopt)") {
		const std::hash<cow_optional<int, use_inline_storage>> hash;

		CHECK(hash(nullopt) == 0);
	}
	SECTION("calling hash(cow_optional)") {
		using cow_optional_type = cow_optional<int, use_inline_storage>;
		const std::hash<cow_optional_type> hash;

		CHECK(hash(cow_optional_type{546}) != 0);
	}
}

// ## cow_use_inline_storage

#if __cpp_lib_optional
TEST_CASE("Testing struct cow_use_inline_storage", "[cow_optional]") {
	SECTION("char data type") {
		CHECK(cow_use_inline_storage_v<char>);
	}
	SECTION("non-trivially copyable data type") {
		CHECK_FALSE(cow_use_inline_storage_v<tracker>);
	}
	SECTION("big object data type") {
		CHECK_FALSE(cow_use_inline_storage_v<std::array<char, 1024>>);
	}
	SECTION("shared_ptr data type") {
		CHECK(cow_use_inline_storage_v<std::shared_ptr<tracker>>);
	}
	SECTION("small trivially copyable object data type") {
		CHECK(cow_use_inline_storage_v<std::tuple<char, char>>);
	}
	SECTION("small non-trivially copyable object data type") {
		CHECK_FALSE(cow_use_inline_storage_v<std::tuple<char, tracker>>);
	}
	SECTION("std::tuple from big object data type") {
		CHECK_FALSE(cow_use_inline_storage_v<std::tuple<char, std::array<char, 1024>>>);
	}
	SECTION("std::tuple from shared_ptr data type") {
		CHECK(cow_use_inline_storage_v<std::tuple<std::shared_ptr<tracker>>>);
	}
	SECTION("std::tuple from reference") {
		CHECK(cow_use_inline_storage_v<std::tuple<char&>>);
	}
}
#endif

} // namespace
} // namespace test
} // namespace detail
} // namespace channels
