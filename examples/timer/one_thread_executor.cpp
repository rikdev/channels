#include "one_thread_executor.h"

one_thread_executor::one_thread_executor(cancellation_token_type cancellation_token)
	: cancellation_token_{std::move(cancellation_token)}
	, worker_thread_{[this] { do_tasks(); }}
{
	token_connection_ = cancellation_token_.connect([this]() noexcept { worker_thread_notifier_.notify_one(); });
}

one_thread_executor::~one_thread_executor()
{
	token_connection_.disconnect();
	worker_thread_.join();
}

void one_thread_executor::do_tasks()
{
	for (;;) {
		std::unique_lock<decltype(tasks_mutex_)> lock{tasks_mutex_};
		worker_thread_notifier_.wait(lock);

		if (cancellation_token_.get_value())
			return;

		decltype(tasks_) tasks;
		std::swap(tasks_, tasks);
		lock.unlock();

		for (const task_type& task : tasks) {
			task();
			if (cancellation_token_.get_value())
				return;
		}
	}
}
