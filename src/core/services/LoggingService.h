/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

/*
 *	Exposes an spdlog logger registry. 
 *
 *	Must be implemented in the view layer.
 */

/**
 * The logger for the core layer.
 */
extern std::shared_ptr<spdlog::logger> g_core_logger;

/**
 * The logger for the shared layer.
 */
extern std::shared_ptr<spdlog::logger> g_shared_logger;