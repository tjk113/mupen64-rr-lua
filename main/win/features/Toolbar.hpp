#pragma once

#include <Windows.h>
#include <cstdint>

extern HWND toolbar_hwnd;

/**
 * \brief Sets the toolbar's visibility
 * \param is_visible Whether the toolbar is visible
 */
void toolbar_set_visibility(int32_t is_visible);

/**
 * \brief Notifies the toolbar of emu state changes
 * \param is_running Whether the emulator is running
 * \param is_resumed Whether the emulator is resumed
 */
void toolbar_on_emu_state_changed(int32_t is_running, int32_t is_resumed);
