#pragma once
#include "detail/compatibility/compile_features.h"
#include <exception>
#include <stdexcept>
#include <vector>

namespace channels {

/// The class `callbacks_exception` defines an exception object that aggregates the exceptions thrown by the callback
/// functions.
/// \see channels::channel::apply_value
class callbacks_exception : public std::exception {
public:
	using exceptions_type = std::vector<std::exception_ptr>;

	callbacks_exception() = default;
	explicit callbacks_exception(exceptions_type exceptions) noexcept;

	CHANNELS_NODISCARD const char* what() const noexcept override;

	/// Return pointers to exceptions thrown by callback functions.
	CHANNELS_NODISCARD const exceptions_type& get_exceptions() const noexcept;

private:
	exceptions_type exceptions_;
};

// channel errors

/// This class defines the type of objects thrown as exceptions to report invalid operations on channel objects.
/// \see channels::channel::is_valid
struct channel_error : std::logic_error {
	using logic_error::logic_error;
};

// transmitter errors

/// This class defines the type of objects thrown as exceptions to report invalid operations on `channels::transmitter`
/// objects.
struct transmitter_error : std::logic_error {
	using logic_error::logic_error;
};

} // namespace channels
