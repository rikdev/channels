#pragma once
#include "compile_features.h"
#include <shared_mutex>

namespace channels {
namespace detail {
namespace compatibility {

#ifdef CHANNELS_CPP_LIB_SHARED_MUTEX
using std::shared_mutex; // NOLINT
#else
using shared_mutex = std::shared_timed_mutex;
#endif

} // namespace compatibility
} // namespace detail
} // namespace channels
