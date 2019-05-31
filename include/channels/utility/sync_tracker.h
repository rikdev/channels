#pragma once
#include "../detail/compatibility/compile_features.h"
#include "../detail/compatibility/shared_mutex.h"
#include <memory>
#include <mutex>
#include <stdexcept>

namespace channels {
namespace utility {

/// This class manages tracking callbacks.
/// \see channels::utility::make_tracking_callback
/// In a multi-threaded system, it is necessary to wait for the completion of the callback functions.
/// This class can do it.
///
/// Example:
/// \code
/// ...
/// channels::utility::sync_tracker tracker;
/// auto callback_1 = channels::utility::make_tracking_callback(tracker.get_tracked_object(), [&shared_data] {...});
/// ...
/// auto callback_n = channels::utility::make_tracking_callback(tracker.get_tracked_object(), [&shared_data] {...});
/// ...
/// {
/// 	sync_tracker::unique_lock lock = lock_all();
/// 	// in this scope you can access to shared_data
/// }
/// ...
/// tracker.sync_release(); // after this line all callbacks managed by tracker will be completed and no callbacks will
/// // be called
/// \endcode
class sync_tracker {
public:
	using unique_lock = std::unique_lock<detail::compatibility::shared_mutex>;

	/// \see get_tracked_object
	class tracked_object;

	/// Constructs a `sync_tracker` object.
	/// \post `is_valid() == true`.
	sync_tracker();

	/// Calls sync_release() and destructs this object.
	~sync_tracker() noexcept;

	sync_tracker(const sync_tracker&) = delete;
	sync_tracker& operator=(const sync_tracker&) = delete;
	sync_tracker(sync_tracker&&) = default;
	sync_tracker& operator=(sync_tracker&&) = default;

	/// Returns the object that is passed to the tracking callback
	/// \throw tracker_error If `is_valid() == false`.
	/// \pre `is_valid() == true`.
	CHANNELS_NODISCARD tracked_object get_tracked_object() const;

	/// Waiting until all objects returned by the `tracked_object::lock()` method are destructed and suspends all
	/// `tracked_object::lock()` method calls.
	/// \warning This method also locks other calls to this method.
	/// \return Lock guard. As long as this object is alive all `tracked_object::lock()` methods are suspended.
	///         After the destruction of this object all `tracked_object::lock()` methods continue to run.
	/// \throw tracker_error If `is_valid() == false`.
	/// \pre `is_valid() == true`.
	CHANNELS_NODISCARD unique_lock lock_all() const;

	/// Waiting until all objects returned by the `tracked_object::lock()` method are destructed and all new
	/// `tracked_object::lock()` method calls will return `static_cast<bool>(tracked_object::lock()) == false`.
	/// \warning This method also locks other calls to this method.
	/// \post `is_valid() == false`.
	void sync_release() noexcept;

	/// Checks if the sync_tracker object is not released.
	CHANNELS_NODISCARD bool is_valid() const noexcept;

private:
	struct shared_state;
	using shared_state_ptr = std::shared_ptr<shared_state>;
	shared_state_ptr shared_state_;
};

class sync_tracker::tracked_object {
public:
	using shared_lock = std::shared_lock<detail::compatibility::shared_mutex>;

	tracked_object() = default;
	explicit tracked_object(shared_state_ptr shared_state) noexcept;

	CHANNELS_NODISCARD shared_lock lock() const;

private:
	shared_state_ptr shared_state_;
};

/// This class defines the type of objects thrown as exceptions to report invalid operations on sync_tracker objects.
struct tracker_error : std::logic_error {
	using logic_error::logic_error;
};

} // namespace utility
} // namespace channels
