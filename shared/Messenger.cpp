#include "Messenger.h"

#include <assert.h>
#include <atomic>
#include <print>
#include <thread>

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

    // Whether a message is currently being broadcasted. Used to wait when subscribing.
    std::atomic g_broadcasting = 0;

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
        ++g_broadcasting;

        for (const auto& subscriber : g_subscriber_cache[message])
        {
            subscriber(data);
        }

        --g_broadcasting;
    }

    std::function<void()> subscribe(Message message, t_user_callback callback)
    {
        while (g_broadcasting != 0)
        {
            std::println("[Messenger] Waiting for broadcast to finish before subscribing...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        Subscriber subscriber = {g_uid_accumulator++, callback};

        g_subscribers.emplace_back(message, subscriber);
        rebuild_subscriber_cache();

        return [=]
        {
            std::erase_if(g_subscribers, [=](const auto& pair)
            {
                return pair.second.uid == subscriber.uid;
            });
            rebuild_subscriber_cache();
        };
    }
}
