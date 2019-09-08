#include "utility/sync_tracker.h"
#include <atomic>
#include <mutex>
#include <utility>

namespace channels {
inline namespace utility {

// sync_tracker::shared_state

struct sync_tracker::shared_state {
	std::atomic<bool> blocked{false};
	detail::compatibility::shared_mutex mutex;
};

// sync_tracker

sync_tracker::sync_tracker()
	: shared_state_{std::make_shared<shared_state>()}
{}

sync_tracker::~sync_tracker() noexcept
{
	sync_release();
}

sync_tracker& sync_tracker::operator=(sync_tracker&& other) noexcept
{
	sync_release();
	shared_state_ = std::move(other.shared_state_);

	return *this;
}

sync_tracker::tracked_object sync_tracker::get_tracked_object() const
{
	if (!is_valid())
		throw tracker_error{"Access to released tracker"};

	return tracked_object{shared_state_};
}

sync_tracker::unique_lock sync_tracker::lock_all() const
{
	if (!is_valid())
		throw tracker_error{"Access to released tracker"};

	return unique_lock{shared_state_->mutex};
}

void sync_tracker::sync_release() noexcept
{
	if (!shared_state_)
		return;

	shared_state_->blocked.store(true, std::memory_order_relaxed);

	{
		// waiting for all tracked_objects will unlock mutex
		const std::lock_guard<decltype(shared_state_->mutex)> lock{shared_state_->mutex};
	}

	shared_state_.reset();
}

bool sync_tracker::is_valid() const noexcept
{
	return static_cast<bool>(shared_state_);
}

// sync_tracker::tracked_object

sync_tracker::tracked_object::tracked_object(shared_state_ptr shared_state) noexcept
	: shared_state_{std::move(shared_state)}
{}

sync_tracker::tracked_object::shared_lock sync_tracker::tracked_object::lock() const
{
	if (!shared_state_)
		return shared_lock{};

	if (shared_state_->blocked.load(std::memory_order_relaxed))
		return shared_lock{};

	std::shared_lock<decltype(shared_state_->mutex)> lock{shared_state_->mutex};

	if (shared_state_->blocked.load(std::memory_order_relaxed))
		return shared_lock{};

	return lock;
}

bool sync_tracker::tracked_object::expired() const noexcept
{
	return !shared_state_ || shared_state_->blocked.load(std::memory_order_relaxed);
}

} // namespace utility
} // namespace channels
