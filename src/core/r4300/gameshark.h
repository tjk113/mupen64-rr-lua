/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

void cht_execute();

/**
 * \brief Adds the cheats specified in a file to the execution list, overwriting its previous contents.
 * \param path The cheat collection file to read from.
 * \return Whether the cheats were successfully added.
 */
bool cht_add_from_file(const std::filesystem::path& path);