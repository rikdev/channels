#pragma once
#include "compatibility/compile_features.h"
#include <utility>

namespace channels {
namespace detail {

// This class just keeps a lock and view.
// It is designed to show the contents of containers in a multithreaded environment.
template<typename Lock, typename BaseView>
class CHANNELS_NODISCARD lock_view : public BaseView {
public:
	lock_view() = default;

	template<typename... Args>
	constexpr explicit lock_view(Lock lock, Args&&... base_view_args);

private:
	Lock lock_;
};

// implementation

template<typename Lock, typename BaseView>
template<typename... Args>
constexpr lock_view<Lock, BaseView>::lock_view(Lock lock, Args&&... base_view_args)
	: BaseView{std::forward<Args>(base_view_args)...}
	, lock_{std::move(lock)}
{}

} // namespace detail
} // namespace channels
