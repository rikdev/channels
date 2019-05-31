#pragma once
#include "cancellation_token.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

class one_thread_executor {
public:
	explicit one_thread_executor(cancellation_token_type cancellation_token);
	~one_thread_executor();

	template<typename F>
	void execute(F&& f)
	{
		{
			std::lock_guard<decltype(tasks_mutex_)> tasks_lock{tasks_mutex_};
			tasks_.emplace_back(std::forward<F>(f));
		}
		worker_thread_notifier_.notify_one();
	}

private:
	using task_type = std::function<void()>;

	void do_tasks();

	cancellation_token_type cancellation_token_;
	channels::connection token_connection_;
	std::vector<task_type> tasks_;
	std::mutex tasks_mutex_;
	std::condition_variable worker_thread_notifier_;
	std::thread worker_thread_;
};

template<typename F>
void execute(one_thread_executor* const e, F&& f)
{
	e->execute(std::forward<F>(f));
}
