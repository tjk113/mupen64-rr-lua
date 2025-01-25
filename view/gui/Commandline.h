/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

namespace Cli
{
    /**
     * \brief Initializes the CLI
     */
    void init();

    /**
	 * Gets whether the CLI wants fast-forward to always be enabled.
	 */
	bool wants_fast_forward();
}
#endif
