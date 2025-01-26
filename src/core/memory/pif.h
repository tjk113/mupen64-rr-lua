/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PIF_H
#define PIF_H

void update_pif_write();
void update_pif_read();

extern int32_t frame_advancing;
extern size_t lag_count;

#endif
