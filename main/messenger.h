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
		 * \brief The emulator paused state has changed
		 */
		EmuPausedChanged,

		/**
		 * \brief The video capturing state has changed
		 */
		CapturingChanged,

		/**
		 * \brief The toolbar visibility has changed
		 */
		ToolbarVisibilityChanged,

		/**
		 * \brief The statusbar visibility has changed
		 */
		StatusbarVisibilityChanged,

		/**
		 * \brief The main window size changed
		 */
		SizeChanged,

		/**
		 * \brief The movie loop state changed
		 */
		MovieLoopChanged,

		/**
		 * \brief The VCR read-only state changed
		 */
		ReadonlyChanged,

		/**
		 * \brief The VCR task changed
		 */
		TaskChanged,

		/**
		 * \brief The VCR engine has begun recording a movie
		 */
		MovieRecordingStarted,

		/**
		 * \brief The VCR engine has begun playing back a movie
		 */
		MoviePlaybackStarted,

		/**
		 * \brief The Lua engine has started a script
		 */
		ScriptStarted,

		/**
		 * \brief The main window has been created
		 */
		AppReady,
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
