#pragma once
#include <Windows.h>

namespace MGECompositor
{
	/**
	 * \brief Initializes the MGE compositor
	 */
	void init();

	/**
	 * \brief Whether the MGE compositor is available with the current configuration
	 */
	bool available();

	/**
	 * \brief Creates the MGE control
	 * \param hwnd The control's parent
	 */
	void create(HWND hwnd);

	/**
	 * \brief Updates the game screen
	 */
	void update_screen();

	/**
 	* \brief Writes the MGE compositor's current emulation front buffer into the destination buffer
 	* \param dest The buffer holding video data of size <c>width * height</c>
 	* \param width The buffer's width
 	* \param height The buffer's height
 	*/
	void read_screen(void** dest, long* width, long* height);
}
