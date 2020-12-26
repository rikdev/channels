#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
#endif
#include <channels/aggregating_channel.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <channels/transmitter.h>
#include "tools/exception_helpers.h"
#include "tools/executor.h"
#include <channels/detail/compatibility/compile_features.h>
#include <catch2/catch.hpp>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace channels {
namespace test {
namespace {

// # tools

// ## box_aggregator

template<typename T>
class box_aggregator {
public:
	using value_type = T;

	continuation_status apply_result(T&& result)
	{
		results_.push_back(std::move(result));
		return continuation_status::to_continue;
	}

	continuation_status apply_exception(std::exception_ptr exception)
	{
		exceptions_.push_back(std::move(exception));
		return continuation_status::to_continue;
	}

	constexpr const std::vector<T>& get_results() const noexcept
	{
		return results_;
	}

	constexpr const std::vector<std::exception_ptr>& get_exceptions() const noexcept
	{
		return exceptions_;
	}

private:
	std::vector<T> results_;
	std::vector<std::exception_ptr> exceptions_;
};

template<>
class box_aggregator<void> {
public:
	continuation_status apply_result() noexcept
	{
		++result_calls_number_;
		return continuation_status::to_continue;
	}

	continuation_status apply_exception(std::exception_ptr exception)
	{
		exceptions_.push_back(std::move(exception));
		return continuation_status::to_continue;
	}

	unsigned get_result_calls_number() const noexcept
	{
		return result_calls_number_;
	}

	constexpr const std::vector<std::exception_ptr>& get_exceptions() const noexcept
	{
		return exceptions_;
	}

private:
	unsigned result_calls_number_ = 0;
	std::vector<std::exception_ptr> exceptions_;
};

// ## aggregator wrappers

constexpr unsigned unlimited = std::numeric_limits<decltype(unlimited)>::max();

template<typename Base>
class limited_aggregator {
public:
	using base_type = Base;

	template<typename... Args>
	explicit limited_aggregator(
		const unsigned results_limit = unlimited, const unsigned exceptions_limit = unlimited, Args&&... args)
		: base{std::forward<Args>(args)...}
		, results_limit_{results_limit}
		, exceptions_limit_{exceptions_limit}
	{}

	template<typename T>
	continuation_status apply_result(T&& result)
	{
		const continuation_status status = base.apply_result(std::forward<T>(result));
		return ++results_number_ < results_limit_ ? status : continuation_status::stop;
	}

	continuation_status apply_result()
	{
		const continuation_status status = base.apply_result();
		return ++results_number_ < results_limit_ ? status : continuation_status::stop;
	}

	continuation_status apply_exception(std::exception_ptr exception)
	{
		const continuation_status status = base.apply_exception(std::move(exception));
		return ++exceptions_number_ < exceptions_limit_ ? status : continuation_status::stop;
	}

	Base base;

private:
	unsigned results_limit_ = unlimited;
	unsigned exceptions_limit_ = unlimited;
	unsigned results_number_ = 0;
	unsigned exceptions_number_ = 0;
};

template<typename Base>
class limited_throw_aggregator {
public:
	template<typename... Args>
	explicit limited_throw_aggregator(
		const unsigned results_limit = unlimited, const unsigned exceptions_limit = unlimited, Args&&... args)
		: base{std::forward<Args>(args)...}
		, results_limit_{results_limit}
		, exceptions_limit_{exceptions_limit}
	{}

	template<typename T>
	continuation_status apply_result(T&& result)
	{
		if (++results_number_ > results_limit_)
			throw std::runtime_error{"results limit is exceeded"};

		return base.apply_result(std::forward<T>(result));
	}

	continuation_status apply_result()
	{
		if (++results_number_ > results_limit_)
			throw std::runtime_error{"results limit is exceeded"};

		return base.apply_result();
	}

	continuation_status apply_exception(std::exception_ptr exception)
	{
		if (++exceptions_number_ > exceptions_limit_)
			throw std::runtime_error{"exceptions limit is exceeded"};

		return base.apply_exception(std::move(exception));
	}

	Base base;

private:
	unsigned results_limit_ = unlimited;
	unsigned exceptions_limit_ = unlimited;
	unsigned results_number_ = 0;
	unsigned exceptions_number_ = 0;
};

// ## custom future

template<typename T>
class future {
public:
	explicit future(std::shared_ptr<T> shared_state) noexcept
		: shared_state_{std::move(shared_state)}
	{}

	CHANNELS_NODISCARD T get()
	{
		return std::move(*shared_state_);
	}

private:
	std::shared_ptr<T> shared_state_;
};

template<typename T>
class promise {
public:
	explicit promise(std::shared_ptr<T> shared_state) noexcept
		: shared_state_{std::move(shared_state)}
	{}

	CHANNELS_NODISCARD future<T> get_future()
	{
		return future<T>{shared_state_};
	}

	void set_value(T value)
	{
		*shared_state_ = std::move(value);
	}

	void set_exception(std::exception_ptr)
	{}

private:
	std::shared_ptr<T> shared_state_;
};

// # tests

TEST_CASE("Testing class aggregating_channel", "[aggregating_channel]") {
	SECTION("testing method is_valid") {
		using channel_type = aggregating_channel<void()>;

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
	SECTION("applying aggregator for channel without callbacks") {
		using channel_type = aggregating_channel<int()>;
		transmitter<channel_type> transmitter;

		using aggregator_type = box_aggregator<typename channel_type::aggregator_argument_type>;
		std::future<aggregator_type> future = transmitter.send(aggregator_type{});
		const aggregator_type aggregator = future.get();
		CHECK(aggregator.get_results().empty());
	}
	SECTION("testing with different callbacks") {
		struct callback_result {
			callback_result(const unsigned n, const int a)
				: callback_number{n}
				, argument{a}
			{}

			bool operator==(const callback_result& other) const noexcept
			{
				return callback_number == other.callback_number && argument == other.argument;
			}

			unsigned callback_number;
			int argument;
		};

		using channel_type = aggregating_channel<callback_result(int)>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();
		tools::async_executor executor;

		const connection connection1 = channel.connect(
			&executor,
			executor.make_synchronizable_callback(
				[](int) -> callback_result { throw std::runtime_error{"callback 1 exception"}; }));
		const connection connection2 = channel.connect(
			&executor,
			executor.make_synchronizable_callback([](const int arg) { return callback_result{2, arg}; }));
		const connection connection3 = channel.connect(
			[](int) -> callback_result { throw std::runtime_error{"callback 3 exception"}; });
		const connection connection4 = channel.connect(
			[](const int arg) { return callback_result{4, arg}; });
		const connection connection5 = channel.connect(
			[](int) -> callback_result { throw std::runtime_error{"callback 5 exception"}; });
		const connection connection6 = channel.connect(
			[](const int arg) { return callback_result{6, arg}; });

		SECTION("testing with aggregator that just collects callback result") {
			using aggregator_type = box_aggregator<channel_type::aggregator_argument_type>;
			constexpr int transmitted_value = 1;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{}, transmitted_value);
			executor.resume_callbacks();

			const aggregator_type aggregator = future.get();
			CHECK(aggregator.get_results() ==
				std::vector<callback_result>{{4, transmitted_value}, {6, transmitted_value}, {2, transmitted_value}});

			CHECK(aggregator.get_exceptions().size() == 3u);
			tools::check_throws(aggregator.get_exceptions());
		}
		SECTION("testing with aggregator that stop aggregating by value") {
			using aggregator_type = limited_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;
			constexpr int transmitted_value = 2;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{1}, transmitted_value);
			executor.resume_callbacks();

			const aggregator_type::base_type aggregator = future.get().base;
			CHECK(aggregator.get_results() == std::vector<callback_result>{{4, transmitted_value}});

			CHECK(aggregator.get_exceptions().size() == 1u);
			tools::check_throws(aggregator.get_exceptions());
		}
		SECTION("testing with aggregator that stop aggregating by exception") {
			using aggregator_type = limited_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;
			constexpr int transmitted_value = 3;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{unlimited, 1}, transmitted_value);
			executor.resume_callbacks();

			const aggregator_type::base_type aggregator = future.get().base;
			CHECK(aggregator.get_results().empty());

			CHECK(aggregator.get_exceptions().size() == 1u);
			tools::check_throws(aggregator.get_exceptions());
		}
		SECTION("testing with aggregator that throw exception in apply_result method") {
			using aggregator_type = limited_throw_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{1}, 4);
			executor.resume_callbacks();

			CHECK_THROWS_AS(future.get(), std::runtime_error);
		}
		SECTION("testing with aggregator that throw exception in apply_exception method") {
			using aggregator_type = limited_throw_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{unlimited, 1}, 5);
			executor.resume_callbacks();

			CHECK_THROWS_AS(future.get(), std::runtime_error);
		}
	}
	SECTION("testing with different void callbacks") {
		using channel_type = aggregating_channel<void()>;
		transmitter<channel_type> transmitter;
		const channel_type& channel = transmitter.get_channel();
		tools::async_executor executor;

		const connection connection1 = channel.connect(
			&executor, executor.make_synchronizable_callback([] { throw std::runtime_error{"callback 1 exception"}; }));
		const connection connection2 = channel.connect(
			&executor, executor.make_synchronizable_callback([] {}));
		const connection connection3 = channel.connect([] { throw std::runtime_error{"callback 3 exception"}; });
		const connection connection4 = channel.connect([] {});
		const connection connection5 = channel.connect([] { throw std::runtime_error{"callback 5 exception"}; });
		const connection connection6 = channel.connect([] {});

		SECTION("testing with aggregator that stop aggregating by value") {
			using aggregator_type = limited_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{1});
			executor.resume_callbacks();

			const aggregator_type::base_type aggregator = future.get().base;
			CHECK(aggregator.get_result_calls_number() == 1u);

			CHECK(aggregator.get_exceptions().size() == 1u);
			tools::check_throws(aggregator.get_exceptions());
		}
		SECTION("testing with aggregator that stop aggregating by exception") {
			using aggregator_type = limited_aggregator<box_aggregator<channel_type::aggregator_argument_type>>;

			std::future<aggregator_type> future = transmitter.send(aggregator_type{unlimited, 1});
			executor.resume_callbacks();

			const aggregator_type::base_type aggregator = future.get().base;
			CHECK(aggregator.get_result_calls_number() == 0u);

			CHECK(aggregator.get_exceptions().size() == 1u);
			tools::check_throws(aggregator.get_exceptions());
		}
	}
	SECTION("testing with custom promise") {
		SECTION("void callbacks") {
			using aggregator_type = box_aggregator<void>;

			transmitter<aggregating_channel<void()>> transmitter;

			const connection c = transmitter.get_channel().connect([] {});

			future<aggregator_type> future =
				transmitter.send(aggregator_type{}, promise<aggregator_type>{std::make_shared<aggregator_type>()});

			const aggregator_type aggregator = future.get();
			CHECK(aggregator.get_result_calls_number() == 1);
		}
		SECTION("non-void callbacks") {
			using aggregator_type = box_aggregator<int>;

			transmitter<aggregating_channel<int(int)>> transmitter;

			const connection c = transmitter.get_channel().connect([](const int v) noexcept { return v; });

			future<aggregator_type> future =
				transmitter.send(aggregator_type{}, 1, promise<aggregator_type>{std::make_shared<aggregator_type>()});

			const aggregator_type aggregator = future.get();
			CHECK(aggregator.get_results() == std::vector<int>{1});
		}
	}
	SECTION("testing comparing functions") {
		using channel_type = aggregating_channel<void()>;

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
