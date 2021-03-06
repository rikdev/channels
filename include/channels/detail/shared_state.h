#pragma once
#include "cast_view.h"
#include "compatibility/apply.h"
#include "compatibility/compile_features.h"
#include "shared_state_base.h"
#include <cassert>
#include <cow/optional.h>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace channels {
namespace detail {

// This class keeps resources that are shared between all copies of the channel object (for example callbacks).
template<typename... Ts>
struct shared_state : shared_state_base {
	using shared_value_type = cow::optional<std::tuple<Ts...>>;

	struct connection_result;
	class invocable_socket;
	using invocable_sockets_shared_view = cast_view<sockets_shared_view, invocable_socket>;

	template<typename Callback>
	invocable_socket& connect(Callback&& callback);

	template<typename Executor, typename Callback>
	invocable_socket& connect(Executor&& executor, Callback&& callback);

	invocable_sockets_shared_view get_sockets();
};

// implementation

// shared_state

template<typename... Ts>
class shared_state<Ts...>::invocable_socket : public socket_base {
public:
	void operator()(const shared_value_type& shared_value)
	{
		invoke(shared_value);
	}

protected:
	virtual void invoke(const shared_value_type& shared_value) = 0;
};

template<typename... Ts>
template<typename Callback>
typename shared_state<Ts...>::invocable_socket& shared_state<Ts...>::connect(Callback&& callback)
{
#ifdef CHANNELS_CPP_LIB_IS_INVOCABLE
	static_assert(std::is_invocable_v<Callback, Ts...>, "Callback must be invocable with channel parameters");
#endif

	class immediately_invocable_socket final : public invocable_socket {
	public:
		explicit immediately_invocable_socket(Callback&& callback)
			: callback_{std::forward<Callback>(callback)}
		{}

	private:
		void invoke(const shared_value_type& shared_value) override
		{
			assert(shared_value); // NOLINT

			if (this->is_blocked())
				return;

			compatibility::apply(callback_, *shared_value);
		}

		std::decay_t<Callback> callback_;
	};

	auto socket_ptr = std::make_shared<immediately_invocable_socket>(std::forward<Callback>(callback));
	invocable_socket& socket = *socket_ptr;
	add(std::move(socket_ptr));
	return socket;
}

template<typename... Ts>
template<typename Executor, typename Callback>
typename shared_state<Ts...>::invocable_socket& shared_state<Ts...>::connect(Executor&& executor, Callback&& callback)
{
#ifdef CHANNELS_CPP_LIB_IS_INVOCABLE
	static_assert(std::is_invocable_v<Callback, const Ts&...>, "Callback must be invocable with channel parameters");
#endif

	class deferred_invocable_socket final
		: public invocable_socket
		, public std::enable_shared_from_this<deferred_invocable_socket> {
	public:
		deferred_invocable_socket(Executor&& executor, Callback&& callback)
			: executor_{std::forward<Executor>(executor)}
			, callback_{std::forward<Callback>(callback)}
		{}

	private:
		void invoke(const shared_value_type& shared_value) override
		{
			assert(shared_value); // NOLINT

			auto task = [self = this->shared_from_this(), value = shared_value]() mutable
			{
				if (!self || !value)
					return; // executor call the task more than once

				const auto local_self = std::move(self);
				const auto local_value = std::move(value);

				if (local_self->is_blocked())
					return;

				assert(local_value); // NOLINT
				compatibility::apply(local_self->callback_, *std::move(local_value));
			};

			// if the current proposals for "Uniform function call" and "Execution support library" are accepted,
			// then it will work with system executors
			execute(executor_, std::move(task));
		}

		std::decay_t<Executor> executor_;
		std::decay_t<Callback> callback_;
	};

	auto socket_ptr =
		std::make_shared<deferred_invocable_socket>(std::forward<Executor>(executor), std::forward<Callback>(callback));
	invocable_socket& socket = *socket_ptr;
	add(std::move(socket_ptr));
	return socket;
}

template<typename... Ts>
typename shared_state<Ts...>::invocable_sockets_shared_view shared_state<Ts...>::get_sockets()
{
	return invocable_sockets_shared_view{shared_state_base::get_sockets()};
}

} // namespace detail
} // namespace channels
