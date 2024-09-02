#pragma once
#include <functional>

namespace Dispatcher
{
	/**
	 * Initializes the dispatcher
	 */
	void init();

	/**
	 * Stops the dispatcher
	 */
	void stop();
	
	/**
	 * \brief Executes a function on the UI thread
	 * \param func The function to be executed
	 */
	void invoke_ui(const std::function<void()>& func);

	/**
	 * \brief Executes a function on a background thread
	 * \param func The function to be executed
	 */
	void invoke_async(const std::function<void()>& func);
	
	/**
	 * \brief Executes the pending functions
	 * \remarks Must be called from the UI thread
	 */
	void execute_ui();
}
