#include <channels/utility/tuple_elvis.h>
#include "tools/relation_only.h"
#include <catch2/catch.hpp>
#include <cow/optional.h>
#include <tuple>
#include <utility>

namespace channels {
namespace test {
namespace {

using channels::test::tools::relation_only_int;

TEST_CASE("Testing class tuple_elvis", "[tuple_elvis]") {
	// constructors

	SECTION("calling default constructor") {
		const tuple_elvis<cow::optional<std::tuple<int>>> elvis;

		CHECK_FALSE(elvis);
	}
	SECTION("calling copy constructor") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		const tuple_elvis<optional_data_type> elvis1{optional_data_type{1}};
		const tuple_elvis<optional_data_type> elvis2{elvis1};

		REQUIRE(elvis2);
		CHECK(*elvis2 == 1);
	}
	SECTION("calling move constructor") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		const tuple_elvis<optional_data_type> elvis{tuple_elvis<optional_data_type>{optional_data_type{1}}};

		REQUIRE(elvis);
		CHECK(*elvis == 1);
	}
	SECTION("calling ctor(const Optional&)") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		const optional_data_type data{0};
		const tuple_elvis<optional_data_type> elvis{data};

		CHECK(elvis);
	}
	SECTION("calling ctor(Optional&&)") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		const tuple_elvis<optional_data_type> elvis{optional_data_type{0}};

		CHECK(elvis);
	}
	SECTION("calling explicit ctor(U&&)") {
		struct explicit_contructible {
			explicit explicit_contructible(const int v)
				: value{v}
			{}

			int value;
		};

		const tuple_elvis<cow::optional<std::tuple<explicit_contructible>>> elvis{1};

		REQUIRE(elvis);
		CHECK(elvis->value == 1);
	}
	SECTION("calling implicit ctor(U&&)") {
		struct implicit_contructible {
			implicit_contructible(const int v)
				: value{v}
			{}

			int value;
		};

		auto f = []() -> tuple_elvis<cow::optional<std::tuple<implicit_contructible>>> {
			return 1;
		};

		const auto elvis = f();
		REQUIRE(elvis);
		CHECK(elvis->value == 1);
	}

	// assignments

	SECTION("calling copy assignment operator") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		const tuple_elvis<optional_data_type> elvis1{optional_data_type{1}};
		tuple_elvis<optional_data_type> elvis2;
		elvis2 = elvis1;

		REQUIRE(elvis2);
		CHECK(*elvis2 == 1);
	}
	SECTION("calling move assignment operator") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		tuple_elvis<optional_data_type> elvis;
		elvis = tuple_elvis<optional_data_type>{optional_data_type{1}};

		REQUIRE(elvis);
		CHECK(*elvis == 1);
	}
	SECTION("calling value assignment operator") {
		using optional_data_type = cow::optional<std::tuple<int>>;
		tuple_elvis<optional_data_type> elvis;
		elvis = 5;

		REQUIRE(elvis);
		CHECK(*elvis == 5);
	}

	// method swap

	SECTION("swapping tuple_elvis objects") {
		using tuple_elvis_type = tuple_elvis<cow::optional<std::tuple<int>>>;
		tuple_elvis_type elvis1{1};
		tuple_elvis_type elvis2{2};

		elvis1.swap(elvis2);

		REQUIRE(elvis1);
		CHECK(*elvis1 == 2);
		REQUIRE(elvis2);
		CHECK(*elvis2 == 1);
	}

	// observers

	SECTION("testing operator->") {
		struct S {
			int i = 1;
			int j = 2;
		};

		using data_type = std::tuple<S>;
		using optional_data_type = cow::optional<data_type>;

		SECTION("calling const version") {
			const tuple_elvis<optional_data_type> elvis{data_type{}};

			CHECK(elvis->i == 1);
		}
		SECTION("calling non-const version") {
			tuple_elvis<optional_data_type> elvis{data_type{}};

			CHECK(elvis->j == 2);
		}
	}
	SECTION("testing operator*") {
		SECTION("calling const lvalue version") {
			const tuple_elvis<cow::optional<std::tuple<float, int>>, 1> elvis{std::make_tuple(1.f, 2)};

			CHECK(*elvis == 2);
		}
		SECTION("calling non-const lvalue version") {
			tuple_elvis<cow::optional<std::tuple<float, int, int>>, 1> elvis{std::make_tuple(1.f, 2, 3)};

			CHECK(*elvis == 2);
		}
		SECTION("calling const rvalue version") {
			const tuple_elvis<cow::optional<std::tuple<int>>, 0> elvis{std::make_tuple(1)};

			CHECK(*std::move(elvis) == 1);
		}
		SECTION("calling non-const rvalue version") {
			tuple_elvis<cow::optional<std::tuple<int>>> elvis{std::make_tuple(1)};

			CHECK(*std::move(elvis) == 1);
		}
	}
	SECTION("testing method value") {
		SECTION("testing const lvalue version") {
			SECTION("return value") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(elvis.value() == 1);
			}
			SECTION("throw exception") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis;

				CHECK_THROWS_AS(elvis.value(), cow::bad_optional_access);
			}
		}
		SECTION("testing non-const lvalue version") {
			SECTION("return value") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(elvis.value() == 1);
			}
			SECTION("throw exception") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis;

				CHECK_THROWS_AS(elvis.value(), cow::bad_optional_access);
			}
		}
		SECTION("testing const rvalue version") {
			SECTION("return value") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(std::move(elvis).value() == 1);
			}
			SECTION("throw exception") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis;

				CHECK_THROWS_AS(std::move(elvis).value(), cow::bad_optional_access);
			}
		}
		SECTION("testing non-const rvalue version") {
			SECTION("return value") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(std::move(elvis).value() == 1);
			}
			SECTION("throw exception") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis;

				CHECK_THROWS_AS(std::move(elvis).value(), cow::bad_optional_access);
			}
		}
	}
	SECTION("testing method value_or") {
		SECTION("testing const lvalue version") {
			SECTION("return value") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(elvis.value_or(2) == 1);
			}
			SECTION("return default value") {
				const tuple_elvis<cow::optional<std::tuple<int>>> elvis{cow::nullopt};

				CHECK(elvis.value_or(2) == 2);
			}
		}
		SECTION("testing non-const rvalue version") {
			SECTION("return value") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

				CHECK(std::move(elvis).value_or(2) == 1);
			}
			SECTION("return default value") {
				tuple_elvis<cow::optional<std::tuple<int>>> elvis{cow::nullopt};

				CHECK(std::move(elvis).value_or(2) == 2);
			}
		}
	}
	SECTION("testing method has_value") {
		SECTION("calling with nullopt value") {
			tuple_elvis<cow::optional<std::tuple<int>>> elvis;

			CHECK_FALSE(elvis.has_value());
		}
		SECTION("calling with non-nullopt value") {
			tuple_elvis<cow::optional<std::tuple<int>>> elvis{1};

			CHECK(elvis.has_value());
		}
	}

	SECTION("testing method to_optional") {
		using tuple_elvis_type = tuple_elvis<cow::optional<std::tuple<int>>>;
		using optional_type = cow::optional<int>;

		SECTION("testing const lvalue version") {
			SECTION("calling with nullopt value") {
				const tuple_elvis_type elvis;

				CHECK(elvis.to_optional<optional_type>() == cow::nullopt);
			}
			SECTION("calling with non-nullopt value") {
				const tuple_elvis_type elvis{1};

				CHECK(elvis.to_optional<optional_type>() == 1);
			}
		}
		SECTION("testing non-const lvalue version") {
			SECTION("calling with nullopt value") {
				tuple_elvis_type elvis;

				CHECK(elvis.to_optional<optional_type>() == cow::nullopt);
			}
			SECTION("calling with non-nullopt value") {
				tuple_elvis_type elvis{1};

				CHECK(elvis.to_optional<optional_type>() == 1);
			}
		}
		SECTION("testing const rvalue version") {
			SECTION("calling with nullopt value") {
				const tuple_elvis_type elvis;

				CHECK(std::move(elvis).to_optional<optional_type>() == cow::nullopt);
			}
			SECTION("calling with non-nullopt value") {
				const tuple_elvis_type elvis{1};

				CHECK(std::move(elvis).value() == 1);
			}
		}
		SECTION("testing non-const rvalue version") {
			SECTION("calling with nullopt value") {
				tuple_elvis_type elvis;

				CHECK(std::move(elvis).to_optional<optional_type>() == cow::nullopt);
			}
			SECTION("calling with non-nullopt value") {
				tuple_elvis_type elvis{1};

				CHECK(std::move(elvis).value() == 1);
			}
		}
	}
}

TEST_CASE("Testing tuple_elvis non-member functions", "[tuple_elvis]") {
	SECTION("calling relational operations") {
		using tuple_elvis_type = tuple_elvis<cow::optional<std::tuple<relation_only_int>>>;

		// ## operator==

		SECTION("calling operator==(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK(lhs == rhs);
		}
		SECTION("calling operator==(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK_FALSE(lhs == rhs);
		}
		SECTION("calling operator==(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs == rhs);
		}
		SECTION("calling operator==(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs == rhs);
		}
		SECTION("calling operator==(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(lhs == rhs);
		}
		SECTION("calling operator==(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs == rhs);
		}

		// ## operator!=

		SECTION("calling operator!=(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs != rhs);
		}
		SECTION("calling operator!=(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK(lhs != rhs);
		}
		SECTION("calling operator!=(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK(lhs != rhs);
		}
		SECTION("calling operator!=(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs != rhs);
		}
		SECTION("calling operator!=(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(lhs != rhs);
		}
		SECTION("calling operator!=(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs != rhs);
		}

		// ## operator<

		SECTION("calling operator<(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs < rhs);
		}
		SECTION("calling operator<(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK(lhs < rhs);
		}
		SECTION("calling operator<(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs < rhs);
		}
		SECTION("calling operator<(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs < rhs);
		}
		SECTION("calling operator<(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(lhs < rhs);
		}
		SECTION("calling operator<(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs < rhs);
		}

		// ## operator>

		SECTION("calling operator>(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs > rhs);
		}
		SECTION("calling operator>(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK_FALSE(lhs > rhs);
		}
		SECTION("calling operator>(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK(lhs > rhs);
		}
		SECTION("calling operator>(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs > rhs);
		}
		SECTION("calling operator>(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(lhs > rhs);
		}
		SECTION("calling operator>(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs > rhs);
		}

		// ## operator<=

		SECTION("calling operator<=(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK(lhs <= rhs);
		}
		SECTION("calling operator<=(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK(lhs <= rhs);
		}
		SECTION("calling operator<=(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK_FALSE(lhs <= rhs);
		}
		SECTION("calling operator<=(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs <= rhs);
		}
		SECTION("calling operator<=(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(lhs <= rhs);
		}
		SECTION("calling operator<=(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(lhs <= rhs);
		}

		// ## operator>=

		SECTION("calling operator>=(empty, empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs;

			CHECK(lhs >= rhs);
		}
		SECTION("calling operator>=(empty, non-empty)") {
			const tuple_elvis_type lhs;
			const tuple_elvis_type rhs{relation_only_int{0}};

			CHECK_FALSE(lhs >= rhs);
		}
		SECTION("calling operator>=(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK(lhs >= rhs);
		}
		SECTION("calling operator>=(non-empty, empty)") {
			const tuple_elvis_type lhs{relation_only_int{0}};
			const tuple_elvis_type rhs;

			CHECK(lhs >= rhs);
		}
		SECTION("calling operator>=(eq, eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs >= rhs);
		}
		SECTION("calling operator>=(lt, gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(lhs >= rhs);
		}
		SECTION("calling operator>=(gt, lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(lhs >= rhs);
		}

		// ### operator== with value

		SECTION("calling operator==(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK_FALSE(lhs == relation_only_int{0});
		}
		SECTION("calling operator==(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK_FALSE(relation_only_int{0} == rhs);
		}
		SECTION("calling operator==(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs == relation_only_int{1});
		}
		SECTION("calling operator==(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{1} == rhs);
		}
		SECTION("calling operator==(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs == relation_only_int{2});
		}
		SECTION("calling operator==(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{2} == rhs);
		}
		SECTION("calling operator==(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK_FALSE(lhs == relation_only_int{1});
		}
		SECTION("calling operator==(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(relation_only_int{1} == rhs);
		}

		// ### operator!= with value

		SECTION("calling operator!=(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK(lhs != relation_only_int{0});
		}
		SECTION("calling operator!=(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK(relation_only_int{0} != rhs);
		}
		SECTION("calling operator!=(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs != relation_only_int{1});
		}
		SECTION("calling operator!=(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{1} != rhs);
		}
		SECTION("calling operator!=(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs != relation_only_int{2});
		}
		SECTION("calling operator!=(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{2} != rhs);
		}
		SECTION("calling operator!=(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK(lhs != relation_only_int{1});
		}
		SECTION("calling operator!=(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(relation_only_int{1} != rhs);
		}

		// ### operator< with value

		SECTION("calling operator<(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK(lhs < relation_only_int{0});
		}
		SECTION("calling operator<(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK_FALSE(relation_only_int{0} < rhs);
		}
		SECTION("calling operator<(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs < relation_only_int{1});
		}
		SECTION("calling operator<(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{1} < rhs);
		}
		SECTION("calling operator<(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs < relation_only_int{2});
		}
		SECTION("calling operator<(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{2} < rhs);
		}
		SECTION("calling operator<(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK_FALSE(lhs < relation_only_int{1});
		}
		SECTION("calling operator<(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(relation_only_int{1} < rhs);
		}

		// ### operator<= with value

		SECTION("calling operator<=(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK(lhs <= relation_only_int{0});
		}
		SECTION("calling operator<=(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK_FALSE(relation_only_int{0} <= rhs);
		}
		SECTION("calling operator<=(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs <= relation_only_int{1});
		}
		SECTION("calling operator<=(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{1} <= rhs);
		}
		SECTION("calling operator<=(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs <= relation_only_int{2});
		}
		SECTION("calling operator<=(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{2} <= rhs);
		}
		SECTION("calling operator<=(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK_FALSE(lhs <= relation_only_int{1});
		}
		SECTION("calling operator<=(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK(relation_only_int{1} <= rhs);
		}

		// ### operator> with value

		SECTION("calling operator>(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK_FALSE(lhs > relation_only_int{0});
		}
		SECTION("calling operator>(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK(relation_only_int{0} > rhs);
		}
		SECTION("calling operator>(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs > relation_only_int{1});
		}
		SECTION("calling operator>(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK_FALSE(relation_only_int{1} > rhs);
		}
		SECTION("calling operator>(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs > relation_only_int{2});
		}
		SECTION("calling operator>(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{2} > rhs);
		}
		SECTION("calling operator>(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK(lhs > relation_only_int{1});
		}
		SECTION("calling operator>(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(relation_only_int{1} > rhs);
		}

		// ### operator>= with value

		SECTION("calling operator>=(empty, value)") {
			const tuple_elvis_type lhs;

			CHECK_FALSE(lhs >= relation_only_int{0});
		}
		SECTION("calling operator>=(value, empty)") {
			const tuple_elvis_type rhs;

			CHECK(relation_only_int{0} >= rhs);
		}
		SECTION("calling operator>=(eq, value_eq)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK(lhs >= relation_only_int{1});
		}
		SECTION("calling operator>=(value_eq, eq)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{1} >= rhs);
		}
		SECTION("calling operator>=(lt, value_gt)") {
			const tuple_elvis_type lhs{relation_only_int{1}};

			CHECK_FALSE(lhs >= relation_only_int{2});
		}
		SECTION("calling operator>=(value_gt, lt)") {
			const tuple_elvis_type rhs{relation_only_int{1}};

			CHECK(relation_only_int{2} >= rhs);
		}
		SECTION("calling operator>=(gt, value_lt)") {
			const tuple_elvis_type lhs{relation_only_int{2}};

			CHECK(lhs >= relation_only_int{1});
		}
		SECTION("calling operator>=(value_lt, gt)") {
			const tuple_elvis_type rhs{relation_only_int{2}};

			CHECK_FALSE(relation_only_int{1} >= rhs);
		}
	}

	// # specialized algorithms

	SECTION("calling swap") {
		using tuple_elvis_type = tuple_elvis<cow::optional<std::tuple<int>>>;
		tuple_elvis_type elvis1{1};
		tuple_elvis_type elvis2{2};

		swap(elvis1, elvis2);

		REQUIRE(elvis1);
		CHECK(*elvis1 == 2);
		REQUIRE(elvis2);
		CHECK(*elvis2 == 1);
	}
	SECTION("calling make_tuple_elvis") {
		const tuple_elvis<cow::optional<std::tuple<int, int>>, 1> elvis =
			make_tuple_elvis<1>(cow::make_optional(std::make_tuple(1, 2)));

		REQUIRE(elvis);
		CHECK(*elvis == 2);
	}
	SECTION("testing hash") {
		using tuple_elvis_type = tuple_elvis<cow::optional<std::tuple<int>>>;

		SECTION("calling hash(nullopt)") {
			const tuple_elvis_type elvis;
			const std::hash<tuple_elvis_type> hash;

			CHECK(hash(elvis) == 0);
		}
		SECTION("calling hash(non-nullopt)") {
			const tuple_elvis_type elvis{42};
			const std::hash<tuple_elvis_type> hash;

			CHECK(hash(elvis) != 0);
		}
	}
}

} // namespace
} // namespace test
} // namespace channels
