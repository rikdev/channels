#pragma once
#include <functional>
#include <utility>

namespace channels {
namespace detail {
namespace compatibility {

#if __cpp_lib_invoke
using std::invoke; // NOLINT
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