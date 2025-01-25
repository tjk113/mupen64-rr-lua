/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once
#include <memory>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> g_view_logger;

namespace Loggers
{
    /**
     * Initializes the loggers
     */
    void init();
}
