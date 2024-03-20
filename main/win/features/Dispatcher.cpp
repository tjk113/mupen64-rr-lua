#include "Dispatcher.h"

#include <assert.h>
#include <mutex>
#include <queue>
#include <Windows.h>
#include "../main_win.h"

namespace Dispatcher
{
	std::queue<std::function<void()>> funcs;
	std::mutex dispatcher_mutex;

	void invoke(const std::function<void()>& func)
	{
		std::lock_guard lock(dispatcher_mutex);
		funcs.push(func);
	}

	void execute()
	{
		std::lock_guard lock(dispatcher_mutex);
		while (!funcs.empty())
		{
			funcs.front()();
			funcs.pop();
		}
	}
}
