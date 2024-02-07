#pragma once

#include <Windows.h>
#include <cstdint>

namespace Toolbar
{
	/**
	 * \brief Gets the toolbar handle
	 */
	HWND hwnd();

	/**
	 * \brief Initializes the toolbar
	 */
	void init();
}
