#pragma once
#include "../detail/compatibility/compile_features.h"
#include <type_traits>
#include <utility>

namespace channels {
inline namespace utility {

// inline_executor

/// It runs each task immediately in the caller's thread.
struct inline_executor {
	template<typename Function>
	void add(Function&& task) const;
};

template<typename Function>
void execute(const inline_executor& executor, Function&& task);

// tracking_executor

namespace executors_detail {

#if __cpp_concepts
template<typename T>
concept TemporaryOwnership = requires(T x) {
	static_cast<bool>(x.lock());
	{static_cast<const T&>(x).expired()} -> bool;
};
#endif

} // namespace executors_detail

/// The `tracking_executor` is a wrapper for the executor that call task if
/// `static_cast<bool>(tracked_object.lock()) == true`.
/// While the task is running, the `tracking_executor` keeps the result of `tracked_object.lock()`.
///
/// Example:
/// \code
/// class Consumer : public std::enable_shared_from_this<Consumer> {
/// ...
/// 	void connect(channel_type channel) {
/// 		connection_ = channel.connect(
/// 			channels::utility::make_tracking_executor(weak_from_this()), [this] { on_channel(); });
/// 	}
/// ...
/// private:
/// 	channels::connection connection_;
/// ...
/// \endcode
///
/// \tparam TrackedObject Type of tracked object. This type must match the concept `TemporaryOwnership`.
/// \tparam Executor Type of user-defined executor.
template<typename TrackedObject, typename Executor = inline_executor>
class tracking_executor {
public:
	constexpr explicit tracking_executor(TrackedObject tracked_object, Executor executor = {});

	template<typename Function>
	void add(Function&& task) const
#if __cpp_concepts
		requires executors_detail::TemporaryOwnership<TrackedObject>
#endif
;

private:
	TrackedObject tracked_object_;
	Executor executor_;
};

#if __cpp_deduction_guides
	template<typename TrackedObject, typename Executor>
	tracking_executor(TrackedObject, Executor) -> tracking_executor<TrackedObject, Executor>;
#endif

/// Makes a `tracking_executor` from the `tracked_object` and `executor`.
template<typename TrackedObject, typename Executor = inline_executor>
CHANNELS_NODISCARD constexpr tracking_executor<std::decay_t<TrackedObject>, std::decay_t<Executor>>
make_tracking_executor(TrackedObject&& tracked_object, Executor&& executor = {});

template<typename TrackedObject, typename Executor, typename Function>
void execute(const tracking_executor<TrackedObject, Executor>& executor, Function&& task);

// implementation

// inline_executor

template<typename Function>
void inline_executor::add(Function&& task) const
{
	std::forward<Function>(task)();
}

template<typename Function>
void execute(const inline_executor& executor, Function&& task)
{
	executor.add(std::forward<Function>(task));
}

// tracking_executor

template<typename TrackedObject, typename Executor>
constexpr tracking_executor<TrackedObject, Executor>::tracking_executor(TrackedObject tracked_object, Executor executor)
	: tracked_object_{std::move(tracked_object)}
	, executor_{std::move(executor)}
{}

template<typename TrackedObject, typename Executor>
template<typename Function>
void tracking_executor<TrackedObject, Executor>::add(Function&& task) const
#if __cpp_concepts
	requires executors_detail::TemporaryOwnership<TrackedObject>
#endif
{
	if (tracked_object_.expired())
		return;

	execute(
		executor_,
		[tracked_object = tracked_object_, task = std::forward<Function>(task)]() mutable {
			if (const auto lock = std::move(tracked_object).lock())
				std::move(task)();
		});
}

template<typename TrackedObject, typename Executor>
constexpr tracking_executor<std::decay_t<TrackedObject>, std::decay_t<Executor>>
make_tracking_executor(TrackedObject&& tracked_object, Executor&& executor)
{
	return tracking_executor<std::decay_t<TrackedObject>, std::decay_t<Executor>>{
		std::forward<TrackedObject>(tracked_object), std::forward<Executor>(executor)};
}

template<typename TrackedObject, typename Executor, typename Function>
void execute(const tracking_executor<TrackedObject, Executor>& executor, Function&& task)
{
	executor.add(std::forward<Function>(task));
}

} // namespace utility
} // namespace channels
