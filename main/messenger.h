#pragma once
#include <any>
#include <functional>

namespace Messenger
{
	/**
	 * \brief Types of messages
	 */
	enum class Message
	{
		/**
		 * \brief The emulator launched state has changed
		 */
		EmuLaunchedChanged,

		/**
		 * \brief The video capturing state has changed
		 */
		CapturingChanged,
	};

	/**
	 * \brief Initializes the messenger
	 */
	void init();

	/**
	 * \brief Broadcasts a message to all listeners
	 * \param message The message type
	 * \param data The message data
	 */
	void broadcast(Message message, std::any data);

	/**
	 * \brief Subscribe to a message
	 * \param message The message type to listen for
	 * \param callback The callback to be invoked upon receiving the specified message type
	 * \return A function which, when called, unsubscribes from the message
	 */
	std::function<void()> subscribe(Message message, std::function<void(std::any)> callback);
}
