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
#include <functional>

enum class e_st_job
{
    none,
    save,
    load
};

enum class e_st_medium
{
    slot,
    path,
    memory
};

extern std::filesystem::path st_path;
extern e_st_job savestates_job;
extern e_st_medium st_medium;
extern bool st_skip_dma;
extern bool old_st;
extern bool savestates_job_success;

using t_savestate_save_callback = std::function<void(const std::vector<uint8_t>&)>;
using t_savestate_load_callback = std::function<void(const std::vector<uint8_t>&)>;

/**
 * \brief Gets the path to the save directory
 */
std::filesystem::path get_saves_directory();

/**
 * \brief Gets the path to the current rom's SRAM file
 */
std::filesystem::path get_sram_path();

/**
 * \brief Gets the path to the current rom's EEPROM file
 */
std::filesystem::path get_eeprom_path();

/**
 * \brief Gets the path to the current rom's flashram file
 */
std::filesystem::path get_flashram_path();

/**
 * \brief Gets the path to the current rom's mempak file
 */
std::filesystem::path get_mempak_path();

/**
 * \brief Sets the selected slot
 */
void savestates_set_slot(size_t slot);

/**
 * \brief Gets the selected slot
 */
size_t savestates_get_slot();

/**
 * \brief Initializes the savestate system
 */
void savestates_init();

/**
 * \brief Reads emu state and generates a savestate depending on the st global state
 * \remarks If <c>savestates_job_use_slot</c> is specified, <c>st_slot</c> will be used to construct the savestate's path, otherwise <c>st_path</c> will be used
 * \warning This function must only be called from the emulation thread. Other callers must use the <c>savestates_do_x</c> family.
 */
void savestates_save_immediate();

/**
 * \brief Overwrites emu state from a read savestate depending on the st global state
 * \remarks If <c>savestates_job_use_slot</c> is specified, <c>st_slot</c> will be used to construct the savestate's path, otherwise <c>st_path</c> will be used
 * \warning This function must only be called from the emulation thread. Other callers must use the <c>savestates_do_x</c> family.
 */
void savestates_load_immediate();

/**
 * Creates an in-memory savestate and calls the provided callback with the savestate buffer 
 * \param callback A callback that will be called with the savestate's data
 * \return Whether the operation was successfully initiated.
 */
bool savestates_save_memory(const t_savestate_save_callback& callback);

/**
 * \brief Loads a savestate from an in-memory buffer
 * \param buffer A buffer containing the savestate
 * \param callback A callback that will be called with the savestate's data upon the load operation completing
 * \remarks The operation won't complete immediately
 * \return Whether the operation was successfully initiated.
 */
bool savestates_load_memory(const std::vector<uint8_t>& buffer, const t_savestate_load_callback& callback);

/**
 * \brief Executes a savestate operation to a path
 * \param path The savestate's path
 * \param job The job to set
 * \remarks The operation won't complete immediately
 */
void savestates_do_file(const std::filesystem::path& path, e_st_job job);

/**
 * \brief Executes a savestate operation to a slot
 * \param slot The slot to construct the savestate path with, or -1 if the current one should be used
 * \param job The job to set
 * \remarks The operation won't complete immediately
 */
void savestates_do_slot(int32_t slot, e_st_job job);
