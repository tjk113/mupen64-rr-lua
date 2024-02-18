#pragma once
#include <Windows.h>

namespace MGECompositor
{
	/**
	 * \brief Initializes the MGE compositor
	 */
	void init();

	/**
	 * \brief Creates the MGE control
	 * \param hwnd The control's parent
	 */
	void create(HWND hwnd);

	/**
	 * \brief Updates the game screen
	 */
	void update_screen();
}
