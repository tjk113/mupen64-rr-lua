/**
 * Mupen64 - savestates.h
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/


#pragma once

#include <filesystem>

enum class e_st_job
{
	none,
	save,
	load
};

extern size_t st_slot;
extern std::filesystem::path st_path;
extern e_st_job savestates_job;
extern bool savestates_job_use_slot;
extern bool st_skip_dma;

/**
 * \brief Reads emu state and generates a savestate depending on the st global state
 * \remarks If <c>savestates_job_use_slot</c> is specified, <c>st_slot</c> will be used to construct the savestate's path, otherwise <c>st_path</c> will be used
 *
 * This function should only be called by the core, other callers shall use <c>savestates_exec</c>
 */
void savestates_save_immediate();

/**
 * \brief Overwrites emu state from a read savestate depending on the st global state
 * \remarks If <c>savestates_job_use_slot</c> is specified, <c>st_slot</c> will be used to construct the savestate's path, otherwise <c>st_path</c> will be used
 *
 * This function should only be called by the core, other callers shall use <c>savestates_exec</c>
 */
void savestates_load_immediate();

/**
 * \brief Executes a savestate operation
 * \param path The savestate's path
 * \param job The job to set
 * \param immediate Whether the operation should be performed immediately
 * \remarks If <c>immediate</c> is true, but the emu state changes while the function runs (e.g.: when the game is unpaused and this function is called from non-emu thread), the results will be invalid
 */
void savestates_exec(std::filesystem::path path, e_st_job job, bool immediate);

/**
 * \brief Executes a savestate operation
 * \param slot The slot to construct the savestate path with
 * \param job The job to set
 * \param immediate Whether the operation should be performed immediately
 * \remarks If <c>immediate</c> is true, but the emu state changes while the function runs (e.g.: when the game is unpaused and this function is called from non-emu thread), the results will be invalid
 */
void savestates_exec(size_t slot, e_st_job job, bool immediate);
