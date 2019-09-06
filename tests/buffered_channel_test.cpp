#include <channels/buffered_channel.h>
#include <channels/transmitter.h>
#include "tools/callbacks.h"
#include "tools/exception_helpers.h"
#include "tools/executor.h"
#include "tools/tracker.h"
#include <catch2/catch.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace channels {
namespace test {
namespace {

TEST_CASE("Testing class buffered_channel", "[buffered_channel]") {
	SECTION("testing method is_valid") {
		using channel_type = buffered_channel<>;

		SECTION("for channel without shared_state") {
			const channel_type channel;

			CHECK_FALSE(channel.is_valid());
		}
		SECTION("for channel with shared_state") {
			const transmitter<channel_type> transmitter;
			const channel_type& channel = transmitter.get_channel();

			CHECK(channel.is_valid());
		}
	}
	SECTION("connecting callback to invalid channel") {
		const buffered_channel<> channel;

		SECTION("without executor") {
			CHECK_THROWS_AS(channel.connect([] {}), channel_error);
		}
		SECTION("with executor") {
			tools::executor executor;

			CHECK_THROWS_AS(channel.connect(&executor, [] {}), channel_error);
		}
	}
	SECTION("connecting callback without arguments") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			const connection connection1 = channel.connect([&calls_number1] { ++calls_number1; });
			CHECK(calls_number1 == 0u);

			transmitter();
			CHECK(calls_number1 == 1u);

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect([&calls_number2] { ++calls_number2; });
			CHECK(calls_number2 == 1u);

			transmitter();
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			const connection connection1 = channel.connect(&executor, [&calls_number1] { ++calls_number1; });

			transmitter();

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect(&executor, [&calls_number2] { ++calls_number2; });
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 0u);

			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);

			transmitter();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);
			executor.run_all_tasks();
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 2u);
		}
	}
	SECTION("connecting callback with multiple arguments") {
		using namespace std::string_literals;

		using channel_type = buffered_channel<float, std::string>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		auto float_value = 1.f;
		auto string_value = "1"s;

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			const connection connection1 = channel.connect(
				[&calls_number1, &float_value, &string_value](const float f, const std::string& s) {
					CHECK(f == float_value);
					CHECK(s == string_value);
					++calls_number1;
				});
			transmitter(1.f, "1");
			CHECK(calls_number1 == 1u);

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect(
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
					CHECK(f == float_value);
					CHECK(s == string_value);
					++calls_number2;
				});
			CHECK(calls_number2 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			transmitter(2.f, "2");
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			const connection connection1 = channel.connect(
				&executor,
				[&calls_number1, &float_value, &string_value](const float f, const std::string& s) {
					CHECK(f == float_value);
					CHECK(s == string_value);
					++calls_number1;
				});
			transmitter(1.f, "1");
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect(
				&executor,
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
					CHECK(f == float_value);
					CHECK(s == string_value);
					++calls_number2;
				});
			CHECK(calls_number2 == 0u);
			executor.run_all_tasks();
			CHECK(calls_number2 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			transmitter(2.f, "2");
			executor.run_all_tasks();
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 2u);
		}
	}
#if __cpp_lib_apply
	SECTION("connecting member") {
		struct test_struct
		{
			void set_value(int v)
			{
				++calls_number;
				value = v;
			}
			int value = 0;
			unsigned calls_number = 0;
		};

		using channel_type = buffered_channel<test_struct*, int>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			const connection connection1 = channel.connect(&test_struct::set_value);

			test_struct data;
			transmitter(&data, 5);
			CHECK(data.value == 5);
			CHECK(data.calls_number == 1u);

			const connection connection2 = channel.connect(&test_struct::set_value);
			CHECK(data.value == 5);
			CHECK(data.calls_number == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;

			const connection connection1 = channel.connect(&executor, &test_struct::set_value);

			test_struct data;
			transmitter(&data, 5);
			executor.run_all_tasks();
			CHECK(data.value == 5);
			CHECK(data.calls_number == 1u);

			const connection connection2 = channel.connect(&executor, &test_struct::set_value);
			executor.run_all_tasks();
			CHECK(data.value == 5);
			CHECK(data.calls_number == 2u);
		}
	}
#endif
	SECTION("connecting non-regular callback") {
		using namespace std::string_literals;

		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			transmitter();
			unsigned calls_number = 0;
			const connection connection = channel.connect(tools::non_regular_callback{calls_number});
			CHECK(calls_number == 1u);
			transmitter();
			CHECK(calls_number == 2u);
		}
		SECTION("with executor") {
			transmitter();
			tools::executor executor;
			unsigned calls_number = 0;
			const connection connection = channel.connect(&executor, tools::non_regular_callback{calls_number});
			executor.run_all_tasks();
			CHECK(calls_number == 1u);
			transmitter();
			executor.run_all_tasks();
			CHECK(calls_number == 2u);
		}
	}
	SECTION("connecting from callback") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		transmitter();
		unsigned calls_number = 0;
		connection connection1;
		const connection connection2 = channel.connect(
			[&calls_number, &connection1, &channel] {
				if (!connection1.is_connected())
					connection1 = channel.connect([&calls_number] { ++calls_number; });
			});
		transmitter();
		CHECK(calls_number == 2u);
	}
	SECTION("checking callbacks call order") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();
		std::vector<int> order;

		SECTION("without executor") {
			const connection connection1 = channel.connect([&order] { order.push_back(1); });
			const connection connection2 = channel.connect([&order] { order.push_back(2); });
			transmitter();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection1 = channel.connect(&executor, [&order] { order.push_back(1); });
			const connection connection2 = channel.connect(&executor, [&order] { order.push_back(2); });
			transmitter();
			executor.run_all_tasks();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
	}
	SECTION("callbacks throw exception (without executor only)") {
		using channel_type = buffered_channel<int>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		unsigned calls_number1 = 0;
		const connection connection1 = channel.connect(
			[&calls_number1](const int value) {
				CHECK(value == 42);
				++calls_number1;
				throw std::runtime_error{"Callback error 1"};
			});

		unsigned calls_number2 = 0;
		const connection connection2 = channel.connect(
			[&calls_number2](const int value) {
				CHECK(value == 42);
				++calls_number2;
				throw std::runtime_error{"Callback error 2"};
			});

		try {
			transmitter(42);
			FAIL("transmitter must be throw callbacks_exception");
		}
		catch (const callbacks_exception& transmitter_exception) {
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);

			const callbacks_exception::exceptions_type& callbacks_exception = transmitter_exception.get_exceptions();
			CHECK(callbacks_exception.size() == 2u);
			tools::check_throws(callbacks_exception);
		}

		unsigned calls_number3 = 0;
		CHECK_THROWS_AS(
			channel.connect(
				[&calls_number3](const int value) {
					CHECK(value == 42);
					++calls_number3;
					throw std::runtime_error{"Callback error 3"};
				}),
			std::runtime_error);
		CHECK(calls_number3 == 1u);
	}
	SECTION("sending value from callback") {
		tools::executor executor;

		using channel_type = buffered_channel<int>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		transmitter(1);
		unsigned calls_number = 0;
		const connection connection = channel.connect(
			&executor,
			[&calls_number, &transmitter](const int value) {
				++calls_number;
				if (value != 0)
					transmitter(0);
			});
		executor.run_all_tasks();
		executor.run_all_tasks();

		CHECK(calls_number == 2u);
	}
	SECTION("disconnecting") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection1 = channel.connect([&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = channel.connect([&calls_number2] { ++calls_number2; });

			CHECK(connection1.is_connected());
			CHECK(connection2.is_connected());

			connection1.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK(connection2.is_connected());
			transmitter();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			connection2.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			transmitter();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			unsigned calls_number3 = 0;
			connection connection3 = channel.connect([&calls_number3] { ++calls_number3; });
			CHECK(calls_number3 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection1 = channel.connect(&executor, [&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = channel.connect(&executor, [&calls_number2] { ++calls_number2; });

			CHECK(connection1.is_connected());
			CHECK(connection2.is_connected());

			transmitter();
			connection1.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			transmitter();
			connection2.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			unsigned calls_number3 = 0;
			connection connection3 = channel.connect(&executor, [&calls_number3] { ++calls_number3; });
			executor.run_all_tasks();
			CHECK(calls_number3 == 1u);
		}
	}
	SECTION("disconnecting in reverse order") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection1 = channel.connect([&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = channel.connect([&calls_number2] { ++calls_number2; });

			CHECK(connection1.is_connected());
			CHECK(connection2.is_connected());

			connection2.disconnect();
			CHECK(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			transmitter();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			connection1.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			transmitter();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			unsigned calls_number3 = 0;
			connection connection3 = channel.connect([&calls_number3] { ++calls_number3; });
			CHECK(calls_number3 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection1 = channel.connect(&executor, [&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = channel.connect(&executor, [&calls_number2] { ++calls_number2; });

			CHECK(connection1.is_connected());
			CHECK(connection2.is_connected());

			connection2.disconnect();
			transmitter();
			CHECK(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			connection1.disconnect();
			transmitter();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			unsigned calls_number3 = 0;
			connection connection3 = channel.connect(&executor, [&calls_number3] { ++calls_number3; });
			CHECK(calls_number3 == 0u);
			executor.run_all_tasks();
			CHECK(calls_number3 == 1u);
		}
	}
	SECTION("disconnecting from callback") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("direct order") {
			transmitter();
			unsigned calls_number = 0;
			connection connection1 = channel.connect([&calls_number] { ++calls_number; });
			const connection connection2 = channel.connect([&connection1] { connection1.disconnect(); });
			transmitter();
			CHECK(calls_number == 1u);
		}
		SECTION("reverse order") {
			transmitter();
			unsigned calls_number = 0;
			connection connection1;
			const connection connection2 = channel.connect([&connection1] { connection1.disconnect(); });
			connection1 = channel.connect([&calls_number] { ++calls_number; });
			transmitter();
			CHECK(calls_number == 1u);
		}
	}
	SECTION("assigning new connection") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection = channel.connect([&calls_number1] { ++calls_number1; });
			transmitter();

			unsigned calls_number2 = 0;
			connection = channel.connect([&calls_number2] { ++calls_number2; });
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);

			transmitter();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection = channel.connect(&executor, [&calls_number1] { ++calls_number1; });
			transmitter();

			unsigned calls_number2 = 0;
			connection = channel.connect(&executor, [&calls_number2] { ++calls_number2; });
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			transmitter();
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 2u);
		}
	}
	SECTION("destroying channel and transmitter") {
		using channel_type = buffered_channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			connection connection = channel.connect([] {});

			transmitter = decltype(transmitter){};
			CHECK(connection.is_connected());
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number = 0;
			connection connection = channel.connect(&executor, [&calls_number] { ++calls_number; });

			transmitter();
			transmitter = decltype(transmitter){};
			CHECK(connection.is_connected());

			executor.run_all_tasks();
			CHECK(calls_number == 1u);
		}
	}
	SECTION("testing method get_value") {
		using channel_type = buffered_channel<tools::tracker>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		CHECK_FALSE(channel.get_value());

		transmitter(272);
		REQUIRE(channel.get_value());
		const tools::tracker& tracker = std::get<0>(*channel.get_value());
		CHECK(tracker.get_value() == 272);
		CHECK(tracker.get_generation() == 1u);
	}
	SECTION("testing method get_value for non-copyable and non-movable value") {
		struct test_struct {
			explicit test_struct(const int v)
				: f{v}
			{}

			test_struct(const test_struct&) = delete;
			test_struct(test_struct&&) = delete;
			test_struct& operator=(const test_struct&) = delete;
			test_struct& operator=(test_struct&&) = delete;

			const int f;
		};

		using channel_type = buffered_channel<test_struct>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		transmitter(1);
		REQUIRE(channel.get_value());
		CHECK(std::get<0>(*channel.get_value()).f == 1);

		transmitter(2);
		REQUIRE(channel.get_value());
		CHECK(std::get<0>(*channel.get_value()).f == 2);
	}
	SECTION("testing method get_value for invalid channel") {
		const buffered_channel<> channel;

		CHECK_THROWS_AS(channel.get_value(), channel_error);
	}
	SECTION("testing comparing functions") {
		using channel_type = buffered_channel<>;

		SECTION("comparing channels without shared state") {
			CHECK(channel_type{} == channel_type{});
		}
		SECTION("comparing equal channels") {
			transmitter<channel_type> transmitter;
			const channel_type& channel = transmitter.get_channel();

			CHECK(channel == channel);
		}
		SECTION("comparing not equal channels") {
			transmitter<channel_type> transmitter1;
			const channel_type& channel1 = transmitter1.get_channel();
			transmitter<channel_type> transmitter2;
			const channel_type& channel2 = transmitter2.get_channel();

			CHECK_FALSE(channel1 == channel2);
		}
	}
	SECTION("testing is_applicable") {
		SECTION("channel arguments is empty, is_applicable arguments is empty") {
			constexpr bool is_applicable = buffered_channel<>::is_applicable<>;

			CHECK(is_applicable);
		}
		SECTION("channel arguments isn't empty (but default constructible), is_applicable arguments is empty") {
			constexpr bool is_applicable = buffered_channel<int>::is_applicable<>;

			CHECK(is_applicable);
		}
		SECTION("channel arguments is empty, is_applicable arguments isn't empty") {
			constexpr bool is_applicable = buffered_channel<>::is_applicable<int>;

			CHECK_FALSE(is_applicable);
		}
		SECTION("channel arguments is constructible from is_applicable arguments") {
			constexpr bool is_applicable = buffered_channel<std::string, double>::is_applicable<const char*, float>;

			CHECK(is_applicable);
		}
		SECTION("channel arguments isn't constructible from is_applicable arguments") {
			constexpr bool is_applicable = buffered_channel<std::string, double>::is_applicable<int, float>;

			CHECK_FALSE(is_applicable);
		}
	}
}

} // namespace
} // namespace test
} // namespace channels
