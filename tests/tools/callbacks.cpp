#include "callbacks.h"

namespace channels {
namespace test {
namespace tools {

// callback_function

void callback_function(unsigned& calls_number) noexcept
{
	++calls_number;
}

// non_regular_callback

non_regular_callback::non_regular_callback(unsigned& calls_number) noexcept
	: calls_number_{calls_number}
{}

void non_regular_callback::operator()()
{
	++calls_number_;
}

} // namespace tools
} // namespace test
} // namespace channels
