#include "AsyncExecutor.h"

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <shared/services/LoggingService.h>

#include "Config.hpp"

std::deque<std::pair<size_t, std::function<void()>>> g_task_queue;
std::mutex g_queue_mutex;
std::condition_variable g_task_cv;

std::atomic g_async_stop = false;
std::thread g_async_thread;

void async_executor_thread()
{
    while (!g_async_stop)
    {
        std::function<void()> task;
        {
            std::unique_lock lock(g_queue_mutex);
            g_task_cv.wait(lock, [] { return g_async_stop || !g_task_queue.empty(); });

            if (g_async_stop && g_task_queue.empty())
                return;

            auto [_, func] = std::move(g_task_queue.front());
            task = func;

            g_task_queue.pop_front();
        }

        if (task)
        {
            task();
        }
    }
}

void AsyncExecutor::init()
{
    g_async_stop = false;
    g_async_thread = std::thread(async_executor_thread);
}

void AsyncExecutor::stop()
{
    {
        std::lock_guard lock(g_queue_mutex);
        g_async_stop = true;
    }
    g_task_cv.notify_all();
    g_async_thread.join();
}

void AsyncExecutor::invoke_async(const std::function<void()>& func, size_t key)
{
    if (!g_config.use_async_executor)
    {
        std::thread(func).detach();
        return;
    }

    {
        std::lock_guard lock(g_queue_mutex);

        if (key)
        {
            for (auto& task_key : g_task_queue | std::views::keys)
            {
                if (task_key == key)
                {
                    g_shared_logger->info("[AsyncExecutor] Function with key %u already exists in the queue.", key);
                    return;
                }
            }
        }

        g_task_queue.push_back(std::make_pair(key, func));
    }
    g_task_cv.notify_one();
}
