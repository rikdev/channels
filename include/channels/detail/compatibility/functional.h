#pragma once
#include "compile_features.h"
#include <functional>
#include <utility>

namespace channels {
namespace detail {
namespace compatibility {

#ifdef CHANNELS_CPP_LIB_INVOKE
using std::invoke; // NOLINT(misc-unused-using-decls)
#else
template<typename F, typename... Args>
decltype(auto) invoke(F&& f, Args&&... args) noexcept(noexcept(std::forward<F>(f)(std::forward<Args>(args)...)))
{
	// it doesn't work for method pointers
	return std::forward<F>(f)(std::forward<Args>(args)...);
}
#endif

} // namespace compatibility
} // namespace detail
} // namespace channels
