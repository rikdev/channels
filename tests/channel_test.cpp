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
			const channel_type& channel = transmitter.get_channel();

			CHECK(channel.is_valid());
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
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number = 0;
			const connection connection = channel.connect([&calls_number] { ++calls_number; });
			CHECK(calls_number == 0u);

			transmitter.send();
			CHECK(calls_number == 1u);

			transmitter.send();
			CHECK(calls_number == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;
			unsigned calls_number = 0;
			const connection connection = channel.connect(&executor, [&calls_number] { ++calls_number; });

			transmitter.send();
			CHECK(calls_number == 0u);
			executor.run_all_tasks();
			CHECK(calls_number == 1u);

			transmitter.send();
			CHECK(calls_number == 1u);
			executor.run_all_tasks();
			CHECK(calls_number == 2u);
		}
	}
	SECTION("connecting callback with reference argument") {
		using channel_type = channel<unsigned&>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			const connection connection = channel.connect(tools::callback_function);

			unsigned calls_number = 0;
			transmitter.send(calls_number);
			CHECK(calls_number == 1u);

			transmitter.send(calls_number);
			CHECK(calls_number == 2u);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection = channel.connect(&executor, tools::callback_function);

			unsigned calls_number = 0;
			transmitter.send(calls_number);
			executor.run_all_tasks();
			CHECK(calls_number == 1u);

			transmitter.send(calls_number);
			executor.run_all_tasks();
			CHECK(calls_number == 2u);
		}
	}
	SECTION("connecting callback with multiple arguments") {
		using namespace std::string_literals;

		using channel_type = channel<float, std::string>;
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
			transmitter.send(1.f, "1");
			CHECK(calls_number1 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect(
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number2;
			});
			transmitter.send(2.f, "2");
			CHECK(calls_number1 == 2u);
			CHECK(calls_number2 == 1u);
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
			transmitter.send(1.f, "1");
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);

			float_value = 2.f;
			string_value = "2"s;

			unsigned calls_number2 = 0;
			const connection connection2 = channel.connect(
				&executor,
				[&calls_number2, &float_value, &string_value](const float f, const std::string& s) {
				CHECK(f == float_value);
				CHECK(s == string_value);
				++calls_number2;
			});
			transmitter.send(2.f, "2");
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
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			const connection connection = channel.connect(&test_struct::set_value);

			test_struct data;
			transmitter.send(&data, 5);
			CHECK(data.value == 5);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection = channel.connect(&executor, &test_struct::set_value);

			test_struct data;
			transmitter.send(&data, 5);
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
			transmitter.send();
			CHECK(calls_number == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;
			unsigned calls_number = 0;
			const connection connection = channel.connect(&executor, tools::non_regular_callback{calls_number});
			transmitter.send();
			executor.run_all_tasks();
			CHECK(calls_number == 1u);
		}
	}
	SECTION("connecting from callback") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		unsigned calls_number = 0;
		connection connection1;
		const connection connection2 = channel.connect(
			[&calls_number, &connection1, &channel] {
				if (!connection1.is_connected())
					connection1 = channel.connect([&calls_number] { ++calls_number; });
			});
		transmitter.send();
		transmitter.send();
		CHECK(calls_number == 1u);
	}
	SECTION("checking callbacks call order") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();
		std::vector<int> order;

		SECTION("without executor") {
			const connection connection1 = channel.connect([&order] { order.push_back(1); });
			const connection connection2 = channel.connect([&order] { order.push_back(2); });
			transmitter.send();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
		SECTION("with executor") {
			tools::executor executor;
			const connection connection1 = channel.connect(&executor, [&order] { order.push_back(1); });
			const connection connection2 = channel.connect(&executor, [&order] { order.push_back(2); });
			transmitter.send();
			executor.run_all_tasks();

			REQUIRE(order.size() == 2u);
			CHECK(order[0] == 1);
			CHECK(order[1] == 2);
		}
	}
	SECTION("callbacks throw exception (without executor only)") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		unsigned calls_number1 = 0;
		const connection connection1 = channel.connect(
			[&calls_number1] {
				++calls_number1;
				throw std::runtime_error{"Callback error 1"};
			});

		unsigned calls_number2 = 0;
		const connection connection2 = channel.connect(
			[&calls_number2] {
				++calls_number2;
				throw std::runtime_error{"Callback error 2"};
			});

		try {
			transmitter.send();
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
	SECTION("sending value from callback") {
		using channel_type = channel<int>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		unsigned calls_number = 0;
		const connection connection = channel.connect(
			[&calls_number, &transmitter](const int value) {
				++calls_number;
				if (value != 0)
					transmitter.send(0);
			});

		transmitter.send(1);
		CHECK(calls_number == 2u);
	}
	SECTION("disconnecting") {
		using channel_type = channel<>;
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
			transmitter.send();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			connection2.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			transmitter.send();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection1 = channel.connect(&executor, [&calls_number1] { ++calls_number1; });

			unsigned calls_number2 = 0;
			connection connection2 = channel.connect(&executor, [&calls_number2] { ++calls_number2; });

			CHECK(connection1.is_connected());
			CHECK(connection2.is_connected());

			transmitter.send();
			connection1.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);

			transmitter.send();
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
			transmitter.send();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			connection1.disconnect();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			transmitter.send();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);
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
			transmitter.send();
			CHECK(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);

			connection1.disconnect();
			transmitter.send();
			CHECK_FALSE(connection1.is_connected());
			CHECK_FALSE(connection2.is_connected());
			executor.run_all_tasks();
			CHECK(calls_number1 == 1u);
			CHECK(calls_number2 == 0u);
		}
	}
	SECTION("disconnecting from callback") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("direct order") {
			unsigned calls_number = 0;
			connection connection1 = channel.connect([&calls_number] { ++calls_number; });
			const connection connection2 = channel.connect([&connection1] { connection1.disconnect(); });
			transmitter.send();
			transmitter.send();
			CHECK(calls_number == 1u);
		}
		SECTION("reverse order") {
			unsigned calls_number = 0;
			connection connection1;
			const connection connection2 = channel.connect([&connection1] { connection1.disconnect(); });
			connection1 = channel.connect([&calls_number] { ++calls_number; });
			transmitter.send();
			transmitter.send();
			CHECK(calls_number == 0u);
		}
	}
	SECTION("assigning new connection") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		SECTION("without executor") {
			unsigned calls_number1 = 0;
			connection connection = channel.connect([&calls_number1] { ++calls_number1; });
			unsigned calls_number2 = 0;
			connection = channel.connect([&calls_number2] { ++calls_number2; });

			transmitter.send();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);
		}
		SECTION("with executor") {
			tools::executor executor;

			unsigned calls_number1 = 0;
			connection connection = channel.connect(&executor, [&calls_number1] { ++calls_number1; });
			unsigned calls_number2 = 0;
			connection = channel.connect(&executor, [&calls_number2] { ++calls_number2; });

			transmitter.send();
			executor.run_all_tasks();
			CHECK(calls_number1 == 0u);
			CHECK(calls_number2 == 1u);
		}
	}
	SECTION("destroying channel and transmitter") {
		using channel_type = channel<>;
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
			transmitter.send();

			transmitter = decltype(transmitter){};
			CHECK(connection.is_connected());

			executor.run_all_tasks();
			CHECK(calls_number == 1);
		}
	}
	SECTION("async connects and disconnects") {
		using channel_type = channel<>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		constexpr int async_disconnects_number = 100;
		std::array<connection, async_disconnects_number> connections_to_disconnect;
		for (connection& c : connections_to_disconnect) {
			c = channel.connect([] {});
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
				std::thread([c = &connections_to_connect[i], &channel]{ *c = channel.connect([] {}); });
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
			~notifier()
			{
				if (callback_)
					callback_();
			}

			notifier(notifier&& other)
				: callback_{std::move(other.callback_)}
			{
				other.callback_ = nullptr;
			}

			notifier& operator==(notifier&&) = delete;
			notifier(const notifier&) = delete;
			notifier& operator==(const notifier&) = delete;

		private:
			callback_type callback_;
		};

		tools::executor executor;

		using channel_type = channel<notifier>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();

		const connection connection1 = channel.connect([](const notifier&) {}); // without executor
		const connection connection2 = channel.connect(&executor, [](const notifier&) {}); // with executor

		unsigned calls_number = 0;
		transmitter.send(notifier{[&calls_number] { ++calls_number; }});
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
}

} // namespace
} // namespace test
} // namespace channels
