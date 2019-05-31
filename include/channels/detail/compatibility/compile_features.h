#pragma once

#if __has_cpp_attribute(nodiscard)
#	if __clang__
		// NOLINTNEXTLINE
#		define CHANNELS_NODISCARD __attribute__((warn_unused_result))
#	else
#		define CHANNELS_NODISCARD [[nodiscard]]
#	endif
#else
#	define CHANNELS_NODISCARD
#endif
