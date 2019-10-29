#pragma once

#if defined(__has_include) && __has_include(<version>)
#	include <version>
#endif

#if __has_cpp_attribute(nodiscard)
#	ifdef __clang__
		// NOLINTNEXTLINE
#		define CHANNELS_NODISCARD __attribute__((warn_unused_result))
#	else
#		define CHANNELS_NODISCARD [[nodiscard]]
#	endif
#else
#	define CHANNELS_NODISCARD
#endif

#if (defined(__cpp_lib_apply) && __cpp_lib_apply >= 201603) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_APPLY
#endif

#if (defined(__cpp_lib_invoke) && __cpp_lib_invoke >= 201411) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_INVOKE
#endif

#if (defined(__cpp_lib_is_invocable) && __cpp_lib_is_invocable >= 201703) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_IS_INVOCABLE
#endif

#if (defined(__cpp_lib_logical_traits) && __cpp_lib_logical_traits >= 201510) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_LOGICAL_TRAITS
#endif

#if (defined(__cpp_lib_optional) && __cpp_lib_optional >= 201606) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_OPTIONAL
#endif

#if (defined(__cpp_lib_shared_mutex) && __cpp_lib_shared_mutex >= 201505) || __cplusplus >= 201703L
#	define CHANNELS_CPP_LIB_SHARED_MUTEX
#endif
