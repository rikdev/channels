#include "executor.h"
#include <utility>

namespace channels {
namespace test {
namespace tools {

// executor

void executor::dispatch(task_type task)
{
	tasks_.push_back(std::move(task));
}

void executor::run_all_tasks()
{
	for (const task_type& task : tasks_)
		task();
}

// async_executor

async_executor& async_executor::operator=(async_executor&& other) noexcept
{
	resume_callbacks();

	threads_ = std::move(other.threads_);
	sync_state_ = std::move(other.sync_state_);

	return *this;
}

async_executor::~async_executor()
{
	resume_callbacks();
}

void async_executor::resume_callbacks() noexcept
{
	if (!sync_state_)
		return;

	{
		const std::lock_guard<std::mutex> lock{sync_state_->run_callbacks_mutex};
		sync_state_->run_callbacks = true;
	}
	sync_state_->run_callbacks_notifier.notify_all();
}

} // namespace tools
} // namespace test
} // namespace channels
