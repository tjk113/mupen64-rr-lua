#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>

enum class statusbar_mode
{
	rombrowser,
	emulating,
};

extern HWND statusbar_hwnd;

/**
 * \brief Sets the statusbar's visibility
 * \param is_visible Whether the toolbar is visible
 */
void statusbar_set_visibility(int32_t is_visible);

/**
 * \brief Temporarily shows text in the statusbar
 * \param text The text to be displayed
 * \param section The statusbar section to display the text in
 */
void statusbar_send_text(std::string text, int32_t section = 0);

/**
 * \brief Sets the statusbar's mode
 */
void statusbar_set_mode(statusbar_mode mode);
