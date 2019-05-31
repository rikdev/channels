#include "executor.h"
#include <utility>

namespace channels {
namespace test {
namespace tools {

void executor::dispatch(task_type task)
{
	tasks_.push_back(std::move(task));
}

void executor::run_all_tasks()
{
	for (const task_type& task : tasks_)
		task();
}

} // namespace tools
} // namespace test
} // namespace channels
