#include "Messenger.h"

#include <assert.h>
#include <atomic>

namespace Messenger
{
#ifdef _DEBUG
#define ASSERT_NOT_CHANGING assert(!g_changing)
#else
#define ASSERT_NOT_CHANGING
#endif

    std::vector<std::pair<Message, t_user_callback>> g_subscribers;
    std::unordered_map<Message, std::vector<t_user_callback>> g_subscriber_cache;

    // Safety flag for debugging, used to track if the subscriber list is being modified while broadcasting.
    std::atomic g_changing = false;

    void init()
    {
    }

    /**
     * Rebuilds the subscriber cache from the current subscriber vector.
     */
    void rebuild_subscriber_cache()
    {
        g_subscriber_cache.clear();

        for (const auto& [key, func] : g_subscribers)
        {
            g_subscriber_cache[key].push_back(func);
        }
    }

    void broadcast(const Message message, std::any data)
    {
        ASSERT_NOT_CHANGING;

        for (const auto& subscriber : g_subscriber_cache[message])
        {
            ASSERT_NOT_CHANGING;
            
            subscriber(data);
        }
    }

    std::function<void()> subscribe(Message message, t_user_callback callback)
    {
        g_changing = true;

        auto index = g_subscribers.size();
        g_subscribers.emplace_back(message, callback);
        rebuild_subscriber_cache();

        g_changing = false;

        return [index]
        {
            g_changing = true;

            g_subscribers.erase(g_subscribers.begin() + index);
            rebuild_subscriber_cache();

            g_changing = false;
        };
    }
}
