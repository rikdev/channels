#pragma once
#include "detail/compatibility/compile_features.h"
#include <memory>

namespace channels {

namespace detail {

class shared_state_base;
class socket_base;

} // namespace detail

/// A handler of channel connection.
class CHANNELS_NODISCARD connection {
public:
	/// Constructs a disconnected `connection` object.
	/// \post `is_connected() == false`.
	connection() = default;

	connection(const connection&) = delete;
	connection(connection&& other) noexcept;
	connection& operator=(const connection&) = delete;
	connection& operator=(connection&& other) noexcept;

	/// Breaks the connection and destructs this object.
	~connection() noexcept;

	/// Breaks the connection.
	/// \post `is_connected() == false`.
	void disconnect() noexcept;

	/// Checks if the connection is connected.
	CHANNELS_NODISCARD bool is_connected() const noexcept;

public: // library private interface
	connection(std::shared_ptr<detail::shared_state_base> shared_state, detail::socket_base& socket) noexcept;

private:
	std::shared_ptr<detail::shared_state_base> shared_state_;
	detail::socket_base* socket_{nullptr};
};

} // namespace channels
