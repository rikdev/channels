#pragma once
#include <thread>
#include <utility>

namespace channels {
namespace test {
namespace tools {

template<typename Threads>
void wait_all(Threads& threads)
{
	for (auto& thread : threads)
		thread.join();
}

class joining_thread {
public:
	joining_thread() = default;

	template<typename Function, typename... Args>
	explicit joining_thread(Function&& f, Args&&... args)
		: thread_{std::forward<Function>(f), std::forward<Args>(args)...}
	{}

	joining_thread(const joining_thread&) = delete;
	joining_thread(joining_thread&&) = default;
	joining_thread& operator=(const joining_thread&) = delete;
	joining_thread& operator=(joining_thread&&) = default;

	explicit joining_thread(std::thread&& thread) noexcept
		: thread_(std::move(thread))
	{}

	joining_thread& operator=(std::thread&& thread) noexcept
	{
		thread_ = std::move(thread);
		return *this;
	}

	~joining_thread()
	{
		if (thread_.joinable())
			thread_.join();
	}

private:
	std::thread thread_;
};

} // namespace tools
} // namespace test
} // namespace channels
