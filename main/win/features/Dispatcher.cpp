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
		SendMessage(mainHWND, WM_EXECUTE_DISPATCHER, 0, 0);
	}

	void execute()
	{
		while (!funcs.empty())
		{
			funcs.front()();
			funcs.pop();
		}
	}
}
