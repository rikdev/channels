#pragma once

namespace channels {
namespace test {
namespace tools {

// callback_function

void callback_function(unsigned& calls_number) noexcept;

// non_regular_callback

class non_regular_callback {
public:
	explicit non_regular_callback(unsigned& calls_number) noexcept;

	void operator()();

private:
	unsigned& calls_number_;
};

} // namespace tools
} // namespace test
} // namespace channels
