#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>

namespace Statusbar
{
	/**
	 * \brief Gets the statusbar handle
	 */
	HWND hwnd();

	/**
	 * \brief Initializes the statusbar
	 */
	void init();

	/**
 	* \brief Shows text in the statusbar
 	* \param text The text to be displayed
 	* \param section The statusbar section to display the text in
 	*/
	void post(const std::string& text, int32_t section = 0);
}

