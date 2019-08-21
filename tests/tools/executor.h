#pragma once
#include "thread_helpers.h"
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace channels {
namespace test {
namespace tools {

// executor

class executor {
public:
	using task_type = std::function<void()>;

	void dispatch(task_type task);

	void run_all_tasks();

private:
	std::vector<task_type> tasks_;
};

template<typename Task>
void execute(executor* const executor, Task&& task)
{
	executor->dispatch(std::forward<Task>(task));
}

// async_executor

class async_executor {
public:
	async_executor() = default;

	async_executor(const async_executor&) = delete;
	async_executor(async_executor&&) = default;

	async_executor& operator=(const async_executor&) = delete;
	async_executor& operator=(async_executor&& other) noexcept;

	~async_executor();

	// Creates a wrapper for the `callback` which suspend the execution of the thread before calling the `callback`
	// (at the synchronization point) and resumes it after calling the `resume_callbacks` method.
	template<typename F>
	auto make_synchronizable_callback(F&& callback);

	// Waits until all tasks reach the synchronization point and then resumes tasks execution.
	void resume_callbacks() noexcept;

	template<typename F>
	void dispatch(F&& task);

private:
	struct sync_state;

	std::unique_ptr<sync_state> sync_state_ = std::make_unique<sync_state>();
	std::vector<joining_thread> threads_;
};

template<typename Task>
void execute(async_executor* const executor, Task&& task)
{
	executor->dispatch(std::forward<Task>(task));
}

struct async_executor::sync_state {
	std::mutex ready_callbacks_number_mutex;
	std::size_t ready_callbacks_number = 0;
	std::condition_variable callback_ready_notifier;

	std::mutex run_callbacks_mutex;
	bool run_callbacks = false;
	std::condition_variable run_callbacks_notifier;
};

template<typename F>
auto async_executor::make_synchronizable_callback(F&& callback)
{
	return [callback = std::forward<F>(callback), state = sync_state_.get()](auto&&... args) -> decltype(auto) {
		{
			const std::lock_guard<std::mutex> lock{ state->ready_callbacks_number_mutex };
			++state->ready_callbacks_number;
		}
		state->callback_ready_notifier.notify_one();

		{
			std::unique_lock<std::mutex> lock{ state->run_callbacks_mutex };
			state->run_callbacks_notifier.wait(lock, [state] { return state->run_callbacks; });
		}

		return callback(std::forward<decltype(args)>(args)...);
	};
}

template<typename F>
void async_executor::dispatch(F&& task)
{
	threads_.emplace_back(std::forward<F>(task));

	std::unique_lock<std::mutex> lock{sync_state_->ready_callbacks_number_mutex};
	sync_state_->callback_ready_notifier.wait(
		lock,
		[expected = threads_.size(), state = sync_state_.get()] {
			return state->ready_callbacks_number == expected;
		});
}

} // namespace tools
} // namespace test
} // namespace channels
