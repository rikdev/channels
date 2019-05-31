#pragma once
#include "compatibility/compile_features.h"
#include <atomic>
#include <cassert>
#include <exception>
#include <future>
#include <type_traits>
#include <utility>

namespace channels {
namespace detail {

// Problem:
// aggregaton_channels uses a shared state (its name is execution_shared_state) to share aggregator between callback
// functions that can be executed in different threads. execution_shared_state returns the result to std::promise which
// keeps it in std::future_shared_state to pass the result to std::future. So the creation of execution_shared_state
// object requires two allocations (one for execution_shared_state and one for std::future_shared_state).
// It looks like this:
//      ________________________
//     | execution_shared_state |      __________________________
//     |           std::promise |-----| std::future_shared_state |
//     |________________________|     |__________________________|
//         ________|_________                       |
//        |                  |                      |
//  ____________       ____________           _____________
// | callback 1 | ... | callback n |         | std::future |
// |____________|     |____________|         |_____________|
//
// The idea is to merge execution_shared_state and future_shared_state into one object:
// template<typename T, ...
// class execution_shared_state : public future_shared_state<T> {
// ...
// };
// ...
// template<typename T, ...
// class callback {
// 	callback(std::shared_ptr<execution_shared_state> shared_steate);
// ...
// };
// ...
// template<typename T>
// class future {
// 	explicit future(std::shared_ptr<future_shared_state<T>> shared_state);
// ...
// };
// ...
// It will look like this:
//              ________________________
//             | execution_shared_state |
//             |________________________|
//         __________________|_______________
//        |                  |               |
//  ____________       ____________       ________
// | callback 1 | ... | callback n |     | future |
// |____________|     |____________|     |________|

template<typename T>
class future_shared_state {
public:
	using value_type = T;

	future_shared_state() = default;
	explicit future_shared_state(const T& v) noexcept(std::is_nothrow_copy_constructible<T>::value);
	explicit future_shared_state(T&& v) noexcept(std::is_nothrow_move_constructible<T>::value);

	future_shared_state(const future_shared_state&) = delete;
	future_shared_state(future_shared_state&&) = delete;
	future_shared_state& operator=(const future_shared_state&) = delete;
	future_shared_state& operator=(future_shared_state&&) = delete;

	~future_shared_state() = default;

	CHANNELS_NODISCARD bool is_ready() const noexcept;
	void make_ready(std::exception_ptr exception = nullptr);

	CHANNELS_NODISCARD std::future<T> get_future();

	CHANNELS_NODISCARD value_type& get_value() noexcept;

private:
	value_type value_;
	std::atomic<bool> ready_{false};
	std::promise<T> promise_; // \todo use custom future instead std::promise
};

// implementation

template<typename T>
future_shared_state<T>::future_shared_state(const T& v) noexcept(std::is_nothrow_copy_constructible<T>::value)
	: value_{v}
{}

template<typename T>
future_shared_state<T>::future_shared_state(T&& v) noexcept(std::is_nothrow_move_constructible<T>::value)
	: value_{std::move(v)}
{}

template<typename T>
bool future_shared_state<T>::is_ready() const noexcept
{
	return ready_.load();
}

template<typename T>
void future_shared_state<T>::make_ready(std::exception_ptr exception)
{
	if (ready_.exchange(true))
		throw std::future_error{std::future_errc::promise_already_satisfied};

	if (exception) {
		promise_.set_exception(std::move(exception));
		return;
	}

	promise_.set_value(std::move(value_));
}

template<typename T>
std::future<T> future_shared_state<T>::get_future()
{
	return promise_.get_future();
}

template<typename T>
typename future_shared_state<T>::value_type& future_shared_state<T>::get_value() noexcept
{
	assert(!is_ready()); // NOLINT

	return value_;
}

} // namespace detail
} // namespace channels
