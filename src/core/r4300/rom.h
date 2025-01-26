/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core/CoreTypes.h>

extern uint8_t* rom;
extern size_t rom_size;
extern char rom_md5[33];
extern t_rom_header ROM_HEADER;

/**
 * \brief Reads the specified rom and initializes the rom module's globals
 * \param path The rom's path
 * \return Whether the operation succeeded
 */
bool rom_load(std::filesystem::path path);

/**
 * \param country_code A rom's country code
 * \return The rom's country name
 */
std::wstring country_code_to_country_name(uint16_t country_code);

/**
 * \param country_code A rom's country code
 * \return The maximum amount of VIs per second intended
 */
uint32_t get_vis_per_second(uint16_t country_code);

/**
 * \brief Performs an in-place byte order correction on a rom buffer depending on its header
 * \param rom The rom buffer
 */
void rom_byteswap(uint8_t* rom);
