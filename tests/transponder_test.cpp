#include <channels/utility/transponder.h>
#include <channels/buffered_channel.h>
#include <channels/channel.h>
#include <channels/transmitter.h>
#include "tools/executor.h"
#include <catch2/catch.hpp>
#include <string>
#include <cstddef>

namespace channels {
namespace utility {
namespace test {
namespace {

using channels::test::tools::executor;

TEST_CASE("Testing class transponder", "[transponder]") {
	SECTION("testing default constructor") {
		const transponder<channel<>> transponder;

		CHECK_FALSE(transponder.get_channel().is_valid());
	}
	SECTION("testing constructors with callback") {
		const std::string test_string{"Hello world!"};
		transmitter<channel<std::string>> source_transmitter;
		using channel_type = buffered_channel<std::size_t>;
		const auto callback = [](transmitter<channel_type>& transmitter, const std::string& s) mutable {
				transmitter(s.length());
			};
		channel_type destination_channel;

		SECTION("without executor") {
			const transponder<channel_type> transponder{source_transmitter.get_channel(), callback};
			destination_channel = transponder.get_channel();
			source_transmitter(test_string);
		}
		SECTION("with executor") {
			executor executor;
			const transponder<channel_type> transponder{source_transmitter.get_channel(), &executor, callback};
			destination_channel = transponder.get_channel();
			source_transmitter(test_string);
			executor.run_all_tasks();
		}

		REQUIRE(destination_channel.get_value());
		CHECK(std::get<0>(*destination_channel.get_value()) == test_string.length());
	}
	SECTION("testing assign methods") {
		transmitter<channel<>> source_transmitter;
		using channel_type = buffered_channel<>;
		const auto callback = [](transmitter<channel_type>& transmitter) { transmitter(); };

		transponder<channel_type> transponder;

		SECTION("without executor") {
			transponder.assign(source_transmitter.get_channel(), callback);
			source_transmitter();
		}
		SECTION("with executor") {
			executor executor;
			transponder.assign(source_transmitter.get_channel(), &executor, callback);
			source_transmitter();
			executor.run_all_tasks();
		}

		CHECK(transponder.get_channel().get_value());
	}
	SECTION("testing reassign") {
		transmitter<channel<>> source_transmitter1;
		transmitter<channel<>> source_transmitter2;

		using channel_type = buffered_channel<>;
		channel_type destination_channel1;
		channel_type destination_channel2;

		const auto callback = [](transmitter<channel_type>& transmitter) { transmitter(); };

		SECTION("without executor") {
			transponder<channel_type> transponder{source_transmitter1.get_channel(), callback};
			destination_channel1 = transponder.get_channel();

			transponder.assign(source_transmitter2.get_channel(), callback);
			destination_channel2 = transponder.get_channel();

			source_transmitter1();
			source_transmitter2();
		}
		SECTION("with executor") {
			executor executor;
			transponder<channel_type> transponder{source_transmitter1.get_channel(), &executor, callback};
			destination_channel1 = transponder.get_channel();

			transponder.assign(source_transmitter2.get_channel(), &executor, callback);
			destination_channel2 = transponder.get_channel();

			source_transmitter1();
			source_transmitter2();
			executor.run_all_tasks();
		}

		CHECK_FALSE(destination_channel1.get_value());
		CHECK(destination_channel2.get_value());
	}
	SECTION("testing reset method") {
		transmitter<channel<>> source_transmitter;
		using channel_type = buffered_channel<>;

		unsigned calls_number = 0;
		transponder<channel_type> transponder{
			source_transmitter.get_channel(), [&calls_number](transmitter<channel_type>&) { ++calls_number; }};
		source_transmitter();
		CHECK(transponder.get_channel().is_valid());

		transponder.reset();
		source_transmitter();
		CHECK_FALSE(transponder.get_channel().is_valid());
		CHECK(calls_number == 1);
	}
}
TEST_CASE("Testing make_transform_adaptor", "[transponder]") {
	SECTION("void callback") {
		auto adaptor = make_transform_adaptor([] {});
		transmitter<buffered_channel<>> transmitter;
		adaptor(transmitter);

		CHECK(transmitter.get_channel().get_value());
	}
	SECTION("testing rvalue callback") {
		const std::string test_string{"Foo"};
		auto adaptor = make_transform_adaptor([](const std::string& s) { return std::make_tuple(s.length(), s); });
		using channel_type = buffered_channel<std::size_t, std::string>;
		transmitter<channel_type> transmitter;
		adaptor(transmitter, test_string);

		const channel_type& channel = transmitter.get_channel();
		REQUIRE(channel.get_value());
		CHECK(std::get<0>(*channel.get_value()) == test_string.length());
		CHECK(std::get<1>(*channel.get_value()) == test_string);
	}
}
TEST_CASE("Testing make_filter_adaptor", "[transponder]") {
	transmitter<channel<int>> transmitter;
	std::vector<int> values;
	const connection c = transmitter.get_channel().connect([&values](const int v) { values.push_back(v); });

	auto adaptor = make_filter_adaptor([](const int v) { return v == 5; });
	adaptor(transmitter, 4);
	adaptor(transmitter, 5);
	adaptor(transmitter, 6);

	CHECK(values == std::vector<int>{5});
}

} // namespace
} // namespace test
} // namespace utility
} // namespace channels
