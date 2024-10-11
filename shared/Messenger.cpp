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

    // Represents a subscriber to a message.
    struct Subscriber
    {
        // A unique identifier.
        size_t uid;

        // The callback function.
        t_user_callback cb;
    };

    std::vector<std::pair<Message, Subscriber>> g_subscribers;

    std::unordered_map<Message, std::vector<t_user_callback>> g_subscriber_cache;

    // UID accumulator for generating unique subscriber IDs. Only write operation is increment.
    size_t g_uid_accumulator;

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
            g_subscriber_cache[key].push_back(func.cb);
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

        Subscriber subscriber = {g_uid_accumulator++, callback};

        g_subscribers.emplace_back(message, subscriber);
        rebuild_subscriber_cache();

        g_changing = false;

        return [=]
        {
            g_changing = true;

            std::erase_if(g_subscribers, [=](const auto& pair)
            {
                return pair.second.uid == subscriber.uid;
            });
            rebuild_subscriber_cache();

            g_changing = false;
        };
    }
}
