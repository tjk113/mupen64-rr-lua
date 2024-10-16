#include "Dispatcher.h"

void Dispatcher::invoke(const std::function<void()>& func)
{
    // If the thread id matches the dispatcher's thread id, we don't need to do anything special and can just execute on this thread.
    if (GetCurrentThreadId() == m_thread_id)
    {
        func();
        return;
    }

    std::lock_guard lock(m_ui_mutex);
    m_ui_queue.push(func);
    m_execute_callback();
}

void Dispatcher::execute()
{
    while (!m_ui_queue.empty())
    {
        m_ui_queue.front()();
        m_ui_queue.pop();
    }
}
