#pragma once

namespace channels {
namespace test {
namespace tools {

class tracker {
public:
	tracker() = default;
	explicit tracker(const int value) noexcept;

	tracker(const tracker& other) noexcept;
	tracker(tracker&& other) noexcept;
	tracker& operator=(const tracker& other) noexcept;
	tracker& operator=(tracker&& other) noexcept;

	int get_value() const noexcept;

	unsigned get_copy_generation() const noexcept;
	unsigned get_move_generation() const noexcept;
	unsigned get_generation() const noexcept;

private:
	int value_ = 0;
	unsigned copy_generation_ = 0;
	unsigned move_generation_ = 0;
};

struct derived_tracker : tracker
{
	using tracker::tracker;
};

struct implicit_tracker_constructible_struct {
	implicit_tracker_constructible_struct(const tracker& t) noexcept;
	implicit_tracker_constructible_struct& operator=(const tracker& t) noexcept;
	implicit_tracker_constructible_struct(tracker&& t) noexcept;
	implicit_tracker_constructible_struct& operator=(tracker&& t) noexcept;

	tracker tracker_object;
};

struct explicit_tracker_constructible_struct {
	explicit explicit_tracker_constructible_struct(const tracker& t) noexcept;
	explicit explicit_tracker_constructible_struct(tracker&& t) noexcept;

	tracker tracker_object;
};

} // namespace tools
} // namespace test
} // namespace channels
