#include "error.h"
#include <utility>

namespace channels {

callbacks_exception::callbacks_exception(exceptions_type exceptions) noexcept
	: exceptions_{std::move(exceptions)}
{}

const char* callbacks_exception::what() const noexcept
{
	return "channels::callbacks_exception";
}

const callbacks_exception::exceptions_type& callbacks_exception::get_exceptions() const noexcept
{
	return exceptions_;
}

} // namespace channels
