#include "tracker.h"
#include <utility>

namespace channels {
namespace test {
namespace tools {

// tracker

tracker::tracker(const int value) noexcept
	: value_{value}
{}

tracker::tracker(const tracker& other) noexcept
	: value_{other.value_}
	, copy_generation_{other.copy_generation_ + 1}
	, move_generation_{other.move_generation_}
{}

tracker& tracker::operator=(const tracker& other) noexcept
{
	value_ = other.value_;
	copy_generation_ = other.copy_generation_ + 1;
	move_generation_ = other.move_generation_;
	return *this;
}

tracker::tracker(tracker&& other) noexcept
	: value_{std::move(other.value_)}
	, copy_generation_{other.copy_generation_}
	, move_generation_{other.move_generation_ + 1}
{}

tracker& tracker::operator=(tracker&& other) noexcept
{
	value_ = std::move(other.value_);
	copy_generation_ = other.copy_generation_;
	move_generation_ = other.move_generation_ + 1;
	return *this;
}

int tracker::get_value() const noexcept
{
	return value_;
}

unsigned tracker::get_copy_generation() const noexcept
{
	return copy_generation_;
}

unsigned tracker::get_move_generation() const noexcept
{
	return move_generation_;
}

unsigned tracker::get_generation() const noexcept
{
	return get_copy_generation() + get_move_generation();
}

// implicit_tracker_constructible_struct

implicit_tracker_constructible_struct::implicit_tracker_constructible_struct(const tracker& t) noexcept
	: tracker_object{t}
{}

implicit_tracker_constructible_struct& implicit_tracker_constructible_struct::operator=(const tracker& t) noexcept
{
	tracker_object = t;
	return *this;
}

implicit_tracker_constructible_struct::implicit_tracker_constructible_struct(tracker&& t) noexcept
	: tracker_object{std::move(t)}
{}

implicit_tracker_constructible_struct& implicit_tracker_constructible_struct::operator=(tracker&& t) noexcept
{
	tracker_object = std::move(t);
	return *this;
}

// explicit_tracker_constructible_struct

explicit_tracker_constructible_struct::explicit_tracker_constructible_struct(const tracker& t) noexcept
	: tracker_object{t}
{}

explicit_tracker_constructible_struct::explicit_tracker_constructible_struct(tracker&& t) noexcept
	: tracker_object{std::move(t)}
{}

} // namespace tools
} // namespace test
} // namespace channels
