#pragma once
#include <shared_mutex>

namespace channels {
namespace detail {
namespace compatibility {

#if __cpp_lib_shared_mutex
using std::shared_mutex; // NOLINT
#else
using shared_mutex = std::shared_timed_mutex;
#endif

} // namespace compatibility
} // namespace detail
} // namespace channels
