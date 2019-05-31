#pragma once
#include <exception>
#include <vector>

namespace channels {
namespace test {
namespace tools {

void check_throws(const std::vector<std::exception_ptr>& exceptions);

} // namespace tools
} // namespace test
} // namespace channels
