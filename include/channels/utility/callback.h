#pragma once
#include "../detail/compatibility/compile_features.h"
#include "../detail/compatibility/functional.h"
#include <functional>
#include <utility>

namespace channels {
namespace utility {

#if __cpp_concepts
template<typename T>
concept bool TemporaryOwnership = requires(T x) {
	{static_cast<bool>(x.lock())}
};
#endif

/// Makes an `tracking_callback` from the `tracked_object` and `callback`.
/// `tracking_callback` is a wrapper function object calling `callback` if
/// `static_cast<bool>(tracked_object.lock()) == true`.
/// While the `callback` is running, this function keeps the result of `tracked_object.lock()`.
/// It can be used to centrally manage `callback` calls using tracked_object or extend the lifetime of an object while
/// calling the `callback` if the object is stored in `std::shared_ptr`.
///
/// Example:
/// \code
/// class Consumer : public std::enable_shared_from_this<Consumer> {
/// ...
/// 	void connect(channel_type channel) {
/// 		connection_ = channel.connect(
/// 			channels::utility::make_tracking_callback(weak_from_this(), [this] { on_channel(); }));
/// 	}
/// ...
/// private:
/// 	channels::connection connection_;
/// ...
/// \endcode
///
/// \param tracked_object Reference to the tracked object.
///                       TrackedObject type must match the concept `TemporaryOwnership`.
/// \param callback Reference to the callback object. Callback type must match the concept `std::Invocable`.
template<typename TrackedObject, typename Callback>
CHANNELS_NODISCARD constexpr decltype(auto) make_tracking_callback(TrackedObject&& tracked_object, Callback&& callback)
#if __cpp_concepts
	requires TemporaryOwnership<TrackedObject>
#endif
{
	return
		[tracked_object = std::forward<TrackedObject>(tracked_object), callback = std::forward<Callback>(callback)](
			auto&&... args)
			noexcept(noexcept(detail::compatibility::invoke(callback, std::forward<decltype(args)>(args)...)))
		{
			if (const auto lock = tracked_object.lock()) {
				return detail::compatibility::invoke(callback, std::forward<decltype(args)>(args)...);
			}

			return decltype(detail::compatibility::invoke(callback, std::forward<decltype(args)>(args)...))();
		};
}

} // namespace utility
} // namespace channels
