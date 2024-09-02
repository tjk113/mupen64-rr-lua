#include "AsyncExecutor.h"

#include <mutex>
#include <queue>

std::queue<std::function<void()>> g_async_queue;
std::recursive_mutex g_async_mutex;
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

void AsyncExecutor::init()
{
    g_async_thread = std::thread(async_executor_thread);
}

void AsyncExecutor::stop()
{
    g_async_stop = true;
    g_async_thread.join();
}

void AsyncExecutor::invoke_async(const std::function<void()>& func)
{
    std::lock_guard lock(g_async_mutex);
    g_async_queue.push(func);
}
