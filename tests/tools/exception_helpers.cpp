#include "exception_helpers.h"
#include <catch2/catch.hpp>
#include <stdexcept>

namespace channels {
namespace test {
namespace tools {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4702) // unreachable code
#endif
void check_throws(const std::vector<std::exception_ptr>& exceptions)
{
	for (const std::exception_ptr& e : exceptions) {
		CHECK_THROWS_AS(std::rethrow_exception(e), std::runtime_error);
	}
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace tools
} // namespace test
} // namespace channels
