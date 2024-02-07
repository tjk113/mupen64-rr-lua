#include "messenger.h"

#include <Windows.h>

namespace Messenger
{
	// List of subscribers: pair of uid, message type, and callback
	std::vector<std::tuple<Message, std::function<void(std::any)>>> subscribers;

	CRITICAL_SECTION cs;

	void init()
	{
		InitializeCriticalSection(&cs);
	}

	void broadcast(Message message, std::any data)
	{
		EnterCriticalSection(&cs);
		for (auto subscriber : subscribers)
		{
			if (std::get<0>(subscriber) != message)
			 continue;
			std::get<1>(subscriber)(data);
		}
		LeaveCriticalSection(&cs);
	}

	std::function<void()> subscribe(Message message,
		std::function<void(std::any)> callback)
	{
		auto index = subscribers.size();
		subscribers.emplace_back(message, callback);

		return [index]
		{
			subscribers.erase(subscribers.begin() + index);
		};
	}
}
