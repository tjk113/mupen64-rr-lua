#include "Dispatcher.h"

#include <mutex>
#include <Windows.h>
#include "../main_win.h"

namespace Dispatcher
{
	std::optional<std::function<void()>> current_function;
	std::mutex dispatcher_mutex;

	void invoke(const std::function<void()>& func)
	{
		std::lock_guard lock(dispatcher_mutex);
		current_function = std::make_optional(func);
		SendMessage(mainHWND, WM_EXECUTE_DISPATCHER, 0, 0);
	}

	void execute()
	{
		if (current_function.has_value())
		{
			current_function.value()();
			current_function.reset();
		}
	}
}
