/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <core/include/core_api.h>

extern bool g_st_skip_dma;
extern bool g_st_old;

/**
 * \brief Does the pending savestate work.
 * \warning This function must only be called from the emulation thread. Other callers must use the <c>savestates_do_x</c> family.
 */
void st_do_work();

/**
 * Clears the work queue and the undo savestate.
 */
void st_on_core_stop();
