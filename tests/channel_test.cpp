#include <channels/channel.h>
#include <channels/transmitter.h>
#include "tools/callbacks.h"
#include "tools/exception_helpers.h"
#include "tools/executor.h"
#include "tools/thread_helpers.h"
#include <catch2/catch.hpp>
#include <algorithm>
#include <array>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace channels {
namespace test {
namespace {

TEST_CASE("Testing class channel", "[channel]") {
	SECTION("testing method is_valid") {
		using channel_type = channel<>;

		SECTION("for channel without shared_state") {
			const channel_type channel;

			CHECK_FALSE(channel.is_valid());
		}
		SECTION("for channel with shared_state") {
			const transmitter<channel_type> transmitter;

			CHECK(transmitter.is_valid());
		}
	}
	SECTION("connecting callback to invalid channel") {
		const channel<> channel;

		SECTION("without executor") {
			CHECK_THROWS_AS(channel.connect([] {}), channel_error);
		}
		SECTION("with executor") {
			tools::executor executor;

			CHECK_THROWS_AS(channel.connect(&executor, [] {}), channel_error);
		}
	}
	SECTION("connecting callback without arguments") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			unsigned calls_number = 0;
			const connection connection = transmitter.connect([&calls_number] { ++calls_number; });
			CHECK(calls_number == 0u);

			transmitter();
			CHECK(calls_number == 1u);

			transmitter();
			CHECK(calls_number == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;
			unsigned calls_number = 0;
			const connection connection = transmitter.connect(&executor, [&calls_number] { ++calls_number; });

			transmitter();
			CHECK(calls_number == 0u);
			executor.run_all_tasks();
			CHECK(calls_number == 1u);

			transmitter();
			CHECK(calls_number == 1u);
			executor.run_all_tasks();
			CHECK(calls_number == 2u);
		}
	}
	SECTION("connecting callback with reference argument") {
		using channel_type = channel<unsigned&>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			const connection connection = transmitter.connect(tools::callback_function);

			unsigned calls_number = 0;
			transmitter(calls_number);
			CHECK(calls_number == 1u);

			transmitter(calls_number);
			CHECK(calls_number == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection = transmitter.connect(&executor, tools::callback_function);

			unsigned calls_number = 0;
			transmitter(calls_number);
			executor.run_all_tasks();
			CHECK(calls_number == 1u);

			transmitter(calls_number);
			executor.run_all_tasks();
			CHECK(calls_number == 2u);
		}
	}
	SECTION("connecting callback with multiple arguments") {
		using namespace std::string_literals;

		using channel_type = channel<float, std::string>;
		transmitter<channel_type> transmitter;

		auto float_value = 1.f;
		auto string_value = "1"s;

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			const connection connection1 = transmitter.connect(
				[&calls_number1, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number1;
			});
			transmitter(1.f, "1");
			CHECK(calls_number1 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			unsigned calls_number2 = 0;
			const connection connection2 = transmitter.connect(
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number2;
			});
			transmitter(2.f, "2");
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;
			unsigned calls_number1 = 0;
			const connection connection1 = transmitter.connect(
				&executor,
				[&calls_number1, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number1;
			});
			transmitter(1.f, "1");
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			unsigned calls_number2 = 0;
			const connection connection2 = transmitter.connect(
				&executor,
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number2;
			});
			transmitter(2.f, "2");
			executor.run_all_tasks();
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 1u);
		}
	}
#if __cpp_lib_apply
	SECTION("connecting member") {
		struct test_struct
		{
			void set_value(int v) { value = v; }
			int value = 0;
		};

		using channel_type = channel<test_struct*, int>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			const connection connection = transmitter.connect(&test_struct::set_value);

			test_struct data;
			transmitter(&data, 5);
			CHECK(data.value == 5);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection = transmitter.connect(&executor, &test_struct::set_value);

			test_struct data;
			transmitter(&data, 5);
			executor.run_all_tasks();
			CHECK(data.value == 5);
		}
	}
#endif
	SECTION("connecting non-regular callback") {
		using namespace std::string_literals;

		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number = 0;
			const connection connection = channel.connect(tools::non_regular_callback{calls_number});
			transmitter();
			CHECK(calls_number == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;
			unsigned calls_number = 0;
			const connection connection = channel.connect(&executor, tools::non_regular_callback{calls_number});
			transmitter();
			executor.run_all_tasks();
			CHECK(calls_number == 1u);
		}
	}
	SECTION("checking callbacks call order") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		std::vector<int> order;

		SECTION("without executor") {
			const connection connection1 = transmitter.connect([&order] { order.push_back(1); });
			const connection connection2 = transmitter.connect([&order] { order.push_back(2); });
			transmitter();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection1 = transmitter.connect(&executor, [&order] { order.push_back(1); });
			const connection connection2 = transmitter.connect(&executor, [&order] { order.push_back(2); });
			transmitter();
			executor.run_all_tasks();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
	}
	SECTION("callbacks throw exception (without executor only)") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		unsigned calls_number1 = 0;
		const connection connection1 = transmitter.connect(
			[&calls_number1] {
				++calls_number1;
				throw std::runtime_error{"Callback error 1"};
			});

		unsigned calls_number2 = 0;
		const connection connection2 = transmitter.connect(
			[&calls_number2] {
				++calls_number2;
				throw std::runtime_error{"Callback error 2"};
			});

		try {
			transmitter();
			FAIL("transmitter must be throw callbacks_exception");
		}
		catch (const callbacks_exception& transmitter_exception) {
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 1u);

			const callbacks_exception::exceptions_type& callbacks_exception =
				transmitter_exception.get_exceptions();
			CHECK(callbacks_exception.size() == 2u);
			tools::check_throws(callbacks_exception);
		}
	}
	SECTION("send value from callback") {
		using channel_type = channel<int>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		unsigned calls_number = 0;
		const connection connection = channel.connect(
			[&calls_number, &transmitter](const int value) {
				++calls_number;
				if (value != 0)
					transmitter(0);
			});

		transmitter(1);
		CHECK(calls_number == 2u);
	}
	SECTION("disconnecting") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection1 = transmitter.connect([&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = transmitter.connect([&calls_number2] { ++calls_number2; });

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
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection1 = transmitter.connect(&executor, [&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = transmitter.connect(&executor, [&calls_number2] { ++calls_number2; });

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
		}
	}
	SECTION("disconnecting in reverse order") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection1 = transmitter.connect([&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = transmitter.connect([&calls_number2] { ++calls_number2; });

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
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection1 = transmitter.connect(&executor, [&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = transmitter.connect(&executor, [&calls_number2] { ++calls_number2; });

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
		}
	}
	SECTION("assigning new connection") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection = transmitter.connect([&calls_number1] { ++calls_number1; });
			unsigned calls_number2 = 0;
			connection = transmitter.connect([&calls_number2] { ++calls_number2; });

			transmitter();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection = transmitter.connect(&executor, [&calls_number1] { ++calls_number1; });
			unsigned calls_number2 = 0;
			connection = transmitter.connect(&executor, [&calls_number2] { ++calls_number2; });

			transmitter();
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);
		}
	}
	SECTION("destroying channel and transmitter") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		SECTION("without executor") {
			connection connection = transmitter.connect([] {});

			transmitter = decltype(transmitter){};
			CHECK_FALSE(connection.is_connected());
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number = 0;
			connection connection = transmitter.connect(&executor, [&calls_number] { ++calls_number; });
			transmitter();

			transmitter = decltype(transmitter){};
			CHECK_FALSE(connection.is_connected());

			executor.run_all_tasks();
			CHECK(calls_number == 1);
		}
	}
	SECTION("async connects and disconnects") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;

		constexpr int async_disconnects_number = 100;
		std::array<connection, async_disconnects_number> connections_to_disconnect;
		for (connection& c : connections_to_disconnect) {
			c = transmitter.connect([] {});
		}
		std::array<std::thread, async_disconnects_number> threads_to_disconnect;
		for (size_t i = 0; i < async_disconnects_number; ++i) {
			threads_to_disconnect[i] = std::thread([c = &connections_to_disconnect[i]]{ c->disconnect(); });
		}

		constexpr int async_connects_number = 100;
		std::array<connection, async_connects_number> connections_to_connect;
		std::array<std::thread, async_connects_number> threads_to_connect;
		for (size_t i = 0; i < async_connects_number; ++i) {
			threads_to_connect[i] =
				std::thread([c = &connections_to_connect[i], &transmitter]{ *c = transmitter.connect([] {}); });
		}

		tools::wait_all(threads_to_disconnect);
		tools::wait_all(threads_to_connect);

		const auto real_disconnects_number = std::count_if(
			connections_to_disconnect.begin(),
			connections_to_disconnect.end(),
			[](const connection& c) { return !c.is_connected(); });
		CHECK(real_disconnects_number == async_disconnects_number);

		const auto real_connects_number = std::count_if(
			connections_to_connect.begin(),
			connections_to_connect.end(),
			[](const connection& c) { return c.is_connected(); });
		CHECK(real_connects_number == async_connects_number);
	}
	SECTION("checking destroy sending data after call all callbacks") {
		class notifier {
		public:
			using callback_type = std::function<void()>;

			explicit notifier(callback_type callback) : callback_{std::move(callback)} {}
			~notifier() { callback_(); }

			notifier(notifier&&) = delete;
			notifier& operator==(notifier&&) = delete;
			notifier(const notifier&) = delete;
			notifier& operator==(const notifier&) = delete;

		private:
			callback_type callback_;
		};

		tools::executor executor;

		using channel_type = channel<notifier>;
		transmitter<channel_type> transmitter;

		const connection connection1 = transmitter.connect([](const notifier&) {}); // without executor
		const connection connection2 = transmitter.connect(&executor, [](const notifier&) {}); // with executor

		unsigned calls_number = 0;
		transmitter([&calls_number] { ++calls_number; });
		CHECK(calls_number == 0u);

		executor.run_all_tasks();
		CHECK(calls_number == 1u);
	}
	SECTION("testing comparing functions") {
		using channel_type = channel<>;

		SECTION("comparing channels without shared state") {
			CHECK(channel_type{} == channel_type{});
		}
		SECTION("comparing equal channels") {
			transmitter<channel_type> transmitter;

			CHECK(transmitter == transmitter);
		}
		SECTION("comparing not equal channels") {
			transmitter<channel_type> transmitter1;
			transmitter<channel_type> transmitter2;

			CHECK_FALSE(transmitter1 == transmitter2);
		}
	}
}

} // namespace
} // namespace test
} // namespace channels
