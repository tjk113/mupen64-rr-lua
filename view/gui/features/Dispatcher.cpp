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

    std::queue<std::function<void()>> g_async_queue;
    std::mutex g_async_mutex;
    std::atomic g_async_stop = false;
    std::thread g_async_thread;

    void async_executor_thread()
    {
        while (!g_async_stop)
        {
            {
                std::lock_guard lock(g_async_mutex);
                while (!g_async_queue.empty())
                {
                    g_async_queue.front()();
                    g_async_queue.pop();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void init()
    {
        g_async_thread = std::thread(async_executor_thread);
    }

    void stop()
    {
        g_async_stop = true;
        g_async_thread.join();
    }

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
