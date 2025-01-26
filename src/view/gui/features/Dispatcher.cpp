/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Dispatcher.h"

void Dispatcher::invoke(const std::function<void()>& func)
{
    // If the thread id matches the dispatcher's thread id, we don't need to do anything special and can just execute on this thread.
    if (GetCurrentThreadId() == m_thread_id)
    {
        func();
        return;
    }

    std::lock_guard lock(m_mutex);
    m_queue.push(func);
    m_execute_callback();
}

void Dispatcher::execute()
{
    while (!m_queue.empty())
    {
        m_queue.front()();
        m_queue.pop();
    }
}
