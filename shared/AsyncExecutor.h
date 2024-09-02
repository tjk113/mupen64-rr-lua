#pragma once
#include <functional>

namespace AsyncExecutor
{
    /**
     * Initializes the async executor
     */
    void init();

    /**
     * Stops the async executor
     */
    void stop();
 
    /**
     * \brief Executes a function on a background thread
     * \param func The function to be executed
     */
    void invoke_async(const std::function<void()>& func);
}
