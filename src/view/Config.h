/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core_api.h>

extern core_cfg g_config;
extern const core_cfg g_default_config;
extern std::vector<t_hotkey*> g_config_hotkeys;

/**
 * \brief Initializes the config subsystem
 */
void init_config();

/**
 * \brief Saves the current config state to the config file
 */
void save_config();

/**
 * \brief Restores the config state from the config file
 */
void load_config();
