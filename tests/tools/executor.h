#pragma once
#include <functional>
#include <utility>
#include <vector>

namespace channels {
namespace test {
namespace tools {

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

} // namespace tools
} // namespace test
} // namespace channels
