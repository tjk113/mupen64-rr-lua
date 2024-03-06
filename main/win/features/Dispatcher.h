#pragma once
#include <functional>

namespace Dispatcher
{
	/**
	 * \brief Executes a function on the UI thread
	 * \param func The function to be executed
	 */
	void invoke(const std::function<void()>& func);

	/**
	 * \brief Executes the pending functions
	 * \remarks Must be called from the UI thread
	 */
	void execute();
}
