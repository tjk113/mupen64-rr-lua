#include "Dispatcher.h"

#include <assert.h>
#include <mutex>
#include <queue>
#include <Windows.h>
#include "../Main.h"
#include <shared/services/FrontendService.h>

namespace Dispatcher
{
    std::queue<std::function<void()>> g_ui_queue;
    std::mutex g_ui_mutex;

    void invoke_ui(const std::function<void()>& func)
    {
        if (is_on_gui_thread())
        {
            func();
            return;
        }

        std::lock_guard lock(g_ui_mutex);
        g_ui_queue.push(func);
        SendMessage(g_main_hwnd, WM_EXECUTE_DISPATCHER, 0, 0);
    }

    void execute_ui()
    {
        while (!g_ui_queue.empty())
        {
            g_ui_queue.front()();
            g_ui_queue.pop();
        }
    }
}
