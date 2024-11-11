/**
 * Mupen64 - savestates.c
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

#include "savestates.h"
#include <libdeflate.h>
#include <stdlib.h>
#include <string>
#include <shared/Config.hpp>
#include "flashram.h"
#include <shared/services/LuaService.h>
#include "memory.h"
#include "summercart.h"
#include <shared/Messenger.h>
#include "../r4300/interrupt.h"
#include "../r4300/r4300.h"
#include "../r4300/rom.h"
#include "../r4300/vcr.h"
#include <cassert>
#include <queue>

#include <shared/services/FrontendService.h>

/// Represents a task to be performed by the savestate system.
struct t_savestate_task
{
    struct t_params
    {
        /// The savestate slot.
        /// Valid if the task's medium is <see cref="e_st_medium::slot"/>.
        size_t slot;

        /// The path to the savestate file.
        /// Valid if the task's medium is <see cref="e_st_medium::path"/>.
        std::filesystem::path path;

        /// The buffer containing the savestate data.
        /// Valid if the task's medium is <see cref="e_st_medium::memory"/> and the job is <see cref="e_st_job::load"/>.
        std::vector<uint8_t> buffer;
    };

    /// The job to perform.
    e_st_job job;

    /// The savestate's source or target medium.
    e_st_medium medium;

    /// Callback to invoke when the task finishes. Can be null.
    t_savestate_callback callback = nullptr;

    /// The task's parameters. Only one field in the struct is valid at a time.
    t_params params{};
};

size_t st_slot;

std::recursive_mutex g_task_mutex;
std::vector<t_savestate_task> g_st_tasks;

// st that comes from no delay fix mupen, it has some differences compared to new st:
// - one frame of input is "embedded", that is the pif ram holds already fetched controller info.
// - execution continues at exception handler (after input poll) at 0x80000180.
bool g_st_old;

// enable fixing .st to work for old mupen (and m64plus)
bool fix_new_st = true;

// read savestate save function for more info
// right now its hardcoded to enabled
bool g_st_skip_dma = false;

//last bit seems to be free
enum { new_st_fixed_bit = (1 << 31) };

constexpr int buflen = 1024;
constexpr int first_block_size = 0xA02BB4 - 32; //32 is md5 hash
uint8_t first_block[first_block_size] = {0};

// Demarcator for new screenshot section
char screen_section[] = "SCR";

std::filesystem::path get_saves_directory()
{
    if (g_config.is_default_saves_directory_used)
    {
        return FrontendService::get_app_path().string() + "save\\";
    }
    return g_config.saves_directory;
}

std::filesystem::path get_sram_path()
{
    return std::format("{}{} {}.sra", get_saves_directory().string(), (const char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_eeprom_path()
{
    return std::format("{}{} {}.eep", get_saves_directory().string(), (const char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_flashram_path()
{
    return std::format("{}{} {}.fla", get_saves_directory().string(), (const char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code));
}

std::filesystem::path get_mempak_path()
{
    return std::format("{}{} {}.mpk", get_saves_directory().string(), (const char*)ROM_HEADER.nom, country_code_to_country_name(ROM_HEADER.Country_code));
}

void savestates_init()
{
    Messenger::subscribe(Messenger::Message::EmuLaunchedChanged, [](std::any data)
    {
        auto value = std::any_cast<bool>(data);

        if (value)
        {
            savestates_set_slot(st_slot);
        }
    });
}

void savestates_set_slot(size_t slot)
{
    st_slot = slot;
    Messenger::broadcast(Messenger::Message::SlotChanged, st_slot);
}

size_t savestates_get_slot()
{
    return st_slot;
}

/// <summary>
/// overwrites emu memory with given data. Make sure to call load_eventqueue_infos as well!!!
/// </summary>
/// <param name="firstBlock"></param>
void load_memory_from_buffer(uint8_t* p)
{
    memread(&p, &rdram_register, sizeof(RDRAM_register));
    if (rdram_register.rdram_device_manuf & new_st_fixed_bit)
    {
        rdram_register.rdram_device_manuf &= ~new_st_fixed_bit; //remove the trick
        g_st_skip_dma = true; //tell dma.c to skip it
    }
    memread(&p, &MI_register, sizeof(mips_register));
    memread(&p, &pi_register, sizeof(PI_register));
    memread(&p, &sp_register, sizeof(SP_register));
    memread(&p, &rsp_register, sizeof(RSP_register));
    memread(&p, &si_register, sizeof(SI_register));
    memread(&p, &vi_register, sizeof(VI_register));
    memread(&p, &ri_register, sizeof(RI_register));
    memread(&p, &ai_register, sizeof(AI_register));
    memread(&p, &dpc_register, sizeof(DPC_register));
    memread(&p, &dps_register, sizeof(DPS_register));
    memread(&p, rdram, 0x800000);
    memread(&p, SP_DMEM, 0x1000);
    memread(&p, SP_IMEM, 0x1000);
    memread(&p, PIF_RAM, 0x40);

    char buf[4 * 32];
    memread(&p, buf, 24);
    load_flashram_infos(buf);

    memread(&p, tlb_LUT_r, 0x100000);
    memread(&p, tlb_LUT_w, 0x100000);

    memread(&p, &llbit, 4);
    memread(&p, reg, 32 * 8);
    for (int i = 0; i < 32; i++)
    {
        memread(&p, reg_cop0 + i, 4);
        memread(&p, buf, 4); // for compatibility with old versions purpose
    }
    memread(&p, &lo, 8);
    memread(&p, &hi, 8);
    memread(&p, reg_cop1_fgr_64, 32 * 8);
    memread(&p, &FCR0, 4);
    memread(&p, &FCR31, 4);
    memread(&p, tlb_e, 32 * sizeof(tlb));
    if (!dynacore && interpcore)
        memread(&p, &interp_addr, 4);
    else
    {
        uint32_t target_addr;
        memread(&p, &target_addr, 4);
        for (char& i : invalid_code)
            i = 1;
        jump_to(target_addr)
    }

    memread(&p, &next_interrupt, 4);
    memread(&p, &next_vi, 4);
    memread(&p, &vi_field, 4);
}

std::vector<uint8_t> generate_savestate()
{
    std::vector<uint8_t> b;

    vecwrite(b, rom_md5, 32);

    //if fixing enabled...
    if (fix_new_st)
    {
        //this is code taken from dma.c:dma_si_read(), it finishes up the dma.
        //it copies data from pif (should contain commands and controller states), updates count reg and adds SI interrupt to queue
        //so why does old mupen and mupen64plus dislike savestates without doing this? well in case of mario 64 it leaves pif command buffer uninitialised
        //and it never can poll input properly (hence the inability to frame advance which is handled inside controller read).

        //But we dont want to do this then load such .st and dma again... so I notify mupen about this in .st,
        //since .st is filled up to the brim with data (not even a single unused offset) I have to store one bit in... rdram manufacturer register
        //this 99.999% wont break on any game, and that bit will be cleared by mupen64plus converter as well, so only old old mupen ever sees this trick.

        //update: I stumbled upon a .st that had the bit set, but didn't have SI_INT in queue,
        //so it froze game, so there exists a way to cause that somehow
        if (get_event(SI_INT) == 0) //if there is no interrupt, add it, otherwise dont care
        {
            for (size_t i = 0; i < (64 / 4); i++)
                rdram[si_register.si_dram_addr / 4 + i] = sl(PIF_RAM[i]);
            update_count();
            add_interrupt_event(SI_INT, /*0x100*/0x900);
            rdram_register.rdram_device_manuf |= new_st_fixed_bit;
            g_st_skip_dma = true;
        }
        //hack end
    }
    vecwrite(b, &rdram_register, sizeof(RDRAM_register));
    vecwrite(b, &MI_register, sizeof(mips_register));
    vecwrite(b, &pi_register, sizeof(PI_register));
    vecwrite(b, &sp_register, sizeof(SP_register));
    vecwrite(b, &rsp_register, sizeof(RSP_register));
    vecwrite(b, &si_register, sizeof(SI_register));
    vecwrite(b, &vi_register, sizeof(VI_register));
    vecwrite(b, &ri_register, sizeof(RI_register));
    vecwrite(b, &ai_register, sizeof(AI_register));
    vecwrite(b, &dpc_register, sizeof(DPC_register));
    vecwrite(b, &dps_register, sizeof(DPS_register));
    vecwrite(b, rdram, 0x800000);
    vecwrite(b, SP_DMEM, 0x1000);
    vecwrite(b, SP_IMEM, 0x1000);
    vecwrite(b, PIF_RAM, 0x40);

    char buf[1024];
    save_flashram_infos(buf);
    vecwrite(b, buf, 24);
    vecwrite(b, tlb_LUT_r, 0x100000);
    vecwrite(b, tlb_LUT_w, 0x100000);
    vecwrite(b, &llbit, 4);
    vecwrite(b, reg, 32 * 8);
    for (size_t i = 0; i < 32; i++)
        vecwrite(b, reg_cop0 + i, 8); // *8 for compatibility with old versions purpose
    vecwrite(b, &lo, 8);
    vecwrite(b, &hi, 8);
    vecwrite(b, reg_cop1_fgr_64, 32 * 8);
    vecwrite(b, &FCR0, 4);
    vecwrite(b, &FCR31, 4);
    vecwrite(b, tlb_e, 32 * sizeof(tlb));
    if (!dynacore && interpcore)
        vecwrite(b, &interp_addr, 4);
    else
        vecwrite(b, &PC->addr, 4);

    vecwrite(b, &next_interrupt, 4);
    vecwrite(b, &next_vi, 4);
    vecwrite(b, &vi_field, 4);

    const int len = save_eventqueue_infos(buf);
    vecwrite(b, buf, len);

    // re-recording
    uint32_t movie_active = VCR::get_task() != e_task::idle;
    vecwrite(b, &movie_active, sizeof(movie_active));
    if (movie_active)
    {
        auto movie_freeze = VCR::freeze().value();

        vecwrite(b, &movie_freeze.size, sizeof(movie_freeze.size));
        vecwrite(b, &movie_freeze.uid, sizeof(movie_freeze.uid));
        vecwrite(b, &movie_freeze.current_sample, sizeof(movie_freeze.current_sample));
        vecwrite(b, &movie_freeze.current_vi, sizeof(movie_freeze.current_vi));
        vecwrite(b, &movie_freeze.length_samples, sizeof(movie_freeze.length_samples));
        vecwrite(b, movie_freeze.input_buffer.data(), movie_freeze.input_buffer.size() * sizeof(BUTTONS));
    }

    if (is_mge_available() && g_config.st_screenshot)
    {
        long width;
        long height;
        FrontendService::mge_get_video_size(&width, &height);

        void* video = malloc(width * height * 3);
        FrontendService::mge_copy_video(video);

        vecwrite(b, screen_section, sizeof(screen_section));
        vecwrite(b, &width, sizeof(width));
        vecwrite(b, &height, sizeof(height));
        vecwrite(b, video, width * height * 3);

        free(video);
    }

    return b;
}

void get_effective_paths(const t_savestate_task& task, std::filesystem::path& st_path, std::filesystem::path& sd_path)
{
    sd_path = std::format("{}{}.sd", get_saves_directory().string(), (const char*)ROM_HEADER.nom);

    if (task.medium == e_st_medium::slot)
    {
        st_path = std::format("{}{} {}.st{}", get_saves_directory().string(), (const char*)ROM_HEADER.nom,
                              country_code_to_country_name(ROM_HEADER.Country_code), std::to_string(st_slot));
    }
}

void savestates_save_immediate_impl(const t_savestate_task& task)
{
    const auto start_time = std::chrono::high_resolution_clock::now();

    const auto st = generate_savestate();

    if (task.medium == e_st_medium::slot && g_config.increment_slot)
    {
        if (st_slot >= 9)
        {
            savestates_set_slot(0);
        }
        else
        {
            savestates_set_slot(st_slot + 1);
        }
    }

    if (task.medium == e_st_medium::slot || task.medium == e_st_medium::path)
    {
        // Always save summercart for some reason
        std::filesystem::path new_st_path = task.params.path;
        std::filesystem::path new_sd_path = "";
        get_effective_paths(task, new_st_path, new_sd_path);
        if (g_config.use_summercart) save_summercart(new_sd_path.string().c_str());

        // Generate compressed buffer
        std::vector<uint8_t> compressed_buffer = st;
        const auto compressor = libdeflate_alloc_compressor(6);
        const size_t final_size = libdeflate_gzip_compress(compressor, st.data(), st.size(), compressed_buffer.data(), compressed_buffer.size());
        libdeflate_free_compressor(compressor);
        compressed_buffer.resize(final_size);

        // write compressed st to disk
        FILE* f = fopen(new_st_path.string().c_str(), "wb");

        if (f == nullptr)
        {
            if (task.callback)
            {
                task.callback(Savestate::Result::Failed, st);
            }
            return;
        }

        fwrite(compressed_buffer.data(), compressed_buffer.size(), 1, f);
        fclose(f);

        if (task.medium == e_st_medium::path)
        {
            FrontendService::show_statusbar(std::format("Saved {}", new_st_path.filename().string()).c_str());
        }
        else
        {
            FrontendService::show_statusbar(std::format("Saved slot {}", st_slot + 1).c_str());
        }
    }

    if (task.callback)
    {
        task.callback(Savestate::Result::Ok, st);
    }
    LuaService::call_save_state();
    g_core_logger->info("Savestate saving took {}ms", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
}

/// <summary>
/// First decompresses file into buffer, then before overwriting emulator memory checks if its good
/// </summary>
/// <param name="silence_not_found_error"></param>
void savestates_load_immediate_impl(const t_savestate_task& task)
{
    const auto start_time = std::chrono::high_resolution_clock::now();

    /*rough .st format :
    0x0 - 0xA02BB0 : memory, registers, stuff like that, known size
    0xA02BB4 - ??? : interrupt queue, dynamic size (cap 1kB)
    ??? - ??????   : m64 info, also dynamic, no cap
    More precise info can be seen on github
    */
    char buf[buflen]{};
    //handle to st
    int len;

    std::filesystem::path new_st_path = task.params.path;
    std::filesystem::path new_sd_path = "";
    get_effective_paths(task, new_st_path, new_sd_path);

    if (g_config.use_summercart) load_summercart(new_sd_path.string().c_str());

    std::vector<uint8_t> st_buf;

    switch (task.medium)
    {
    case e_st_medium::slot:
    case e_st_medium::path:
        st_buf = read_file_buffer(new_st_path);
        break;
    case e_st_medium::memory:
        st_buf = task.params.buffer;
        break;
    default: assert(false);
    }

    if (st_buf.empty())
    {
        FrontendService::show_statusbar(std::format("{} not found", new_st_path.filename().string()).c_str());
        if (task.callback)
        {
            task.callback(Savestate::Result::Failed, {});
        }
        return;
    }

    std::vector<uint8_t> decompressed_buf = auto_decompress(st_buf);
    if (decompressed_buf.empty())
    {
        FrontendService::show_error("Failed to decompress savestate", nullptr);
        if (task.callback)
        {
            task.callback(Savestate::Result::Failed, {});
        }
        return;
    }

    // BUG (PRONE): we arent allowed to hold on to a vector element pointer
    // find another way of doing this
    auto ptr = decompressed_buf.data();

    // compare current rom hash with one stored in state
    char md5[33] = {0};
    memread(&ptr, &md5, 32);

    if (memcmp(md5, rom_md5, 32))
    {
        auto result = FrontendService::show_ask_dialog(std::format(
            "The savestate was created on a rom with hash {}, but is being loaded on another rom.\r\nThe emulator may crash. Are you sure you want to continue?",
            md5).c_str());

        if (!result)
        {
            if (task.callback)
            {
                task.callback(Savestate::Result::Failed, {});
            }
            return;
        }
    }

    // new version does one bigass gzread for first part of .st (static size)
    memread(&ptr, first_block, first_block_size);

    // now read interrupt queue into buf
    for (len = 0; len < buflen; len += 8)
    {
        memread(&ptr, buf + len, 4);
        if (*reinterpret_cast<unsigned long*>(&buf[len]) == 0xFFFFFFFF)
            break;
        memread(&ptr, buf + len + 4, 4);
    }
    if (len == buflen)
    {
        // Exhausted the buffer and still no terminator. Prevents the buffer overflow "Queuecrush".
        FrontendService::show_statusbar("Event queue too long (corrupted?)");
        if (task.callback)
        {
            task.callback(Savestate::Result::Failed, {});
        }
        return;
    }

    uint32_t is_movie;
    memread(&ptr, &is_movie, sizeof(is_movie));

    if (is_movie)
    {
        // this .st is part of a movie, we need to overwrite our current movie buffer
        // hash matches, load and verify rest of the data
        t_movie_freeze freeze{};

        memread(&ptr, &freeze.size, sizeof(freeze.size));
        memread(&ptr, &freeze.uid, sizeof(freeze.uid));
        memread(&ptr, &freeze.current_sample, sizeof(freeze.current_sample));
        memread(&ptr, &freeze.current_vi, sizeof(freeze.current_vi));
        memread(&ptr, &freeze.length_samples, sizeof(freeze.length_samples));

        freeze.input_buffer.resize(sizeof(BUTTONS) * (freeze.length_samples + 1));
        memread(&ptr, freeze.input_buffer.data(), freeze.input_buffer.size());

        const auto code = VCR::unfreeze(freeze);

        if (code != VCR::Result::Ok && VCR::get_task() != e_task::idle)
        {
            std::string err_str = "Failed to restore movie, ";
            switch (code)
            {
            case VCR::Result::NotFromThisMovie:
                err_str += "the snapshot is not from this movie.";
                break;
            case VCR::Result::InvalidFrame:
                err_str += "the savestate frame is outside the bounds of the movie.";
                break;
            case VCR::Result::InvalidFormat:
                err_str += "the format is invalid.";
                stop = true;
                break;
            default:
                err_str += "an unknown error has occured.";
                break;
            }
            err_str += "\r\nAre you sure you want to continue?";
            auto result = FrontendService::show_ask_dialog(err_str.c_str(), nullptr, true);
            if (!result)
            {
                VCR::stop_all();
                if (task.callback)
                {
                    task.callback(Savestate::Result::Failed, {});
                }
                goto failedLoad;
            }
        }
    }
    else
    {
        if (VCR::get_task() == e_task::recording || VCR::get_task() == e_task::playback)
        {
            auto result = FrontendService::show_ask_dialog("Loading a non-movie savestate during movie playback might desynchronize playback.\r\nAre you sure you want to continue?");
            if (!result)
            {
                if (task.callback)
                {
                    task.callback(Savestate::Result::Failed, {});
                }
                return;
            }
        }

        // at this point we know the savestate is safe to be loaded (done after else block)
    }
    {
        g_core_logger->info("[Savestates] {} bytes remaining", decompressed_buf.size() - (ptr - decompressed_buf.data()));
        long video_width = 0;
        long video_height = 0;
        void* video_buffer = nullptr;
        if (decompressed_buf.size() - (ptr - decompressed_buf.data()) > 0)
        {
            char scr_section[sizeof(screen_section)] = {0};
            memread(&ptr, scr_section, sizeof(screen_section));

            if (!memcmp(scr_section, screen_section, sizeof(screen_section)))
            {
                g_core_logger->info("[Savestates] Restoring screen buffer...");
                memread(&ptr, &video_width, sizeof(video_width));
                memread(&ptr, &video_height, sizeof(video_height));

                video_buffer = malloc(video_width * video_height * 3);
                memread(&ptr, video_buffer, video_width * video_height * 3);
            }
        }

        // so far loading success! overwrite memory
        load_eventqueue_infos(buf);
        load_memory_from_buffer(first_block);

        // NOTE: We don't want to restore screen buffer while seeking, since it creates a short ugly flicker when the movie restarts by loading state
        if (is_mge_available() && video_buffer && !VCR::is_seeking())
        {
            long current_width, current_height;
            FrontendService::mge_get_video_size(&current_width, &current_height);
            if (current_width == video_width && current_height == video_height)
            {
                FrontendService::mge_load_screen(video_buffer);
                free(video_buffer);
            }
        }
    }
    LuaService::call_load_state();
    if (task.medium == e_st_medium::path)
    {
        FrontendService::show_statusbar(std::format("Loaded {}", new_st_path.filename().string()).c_str());
    }
    if (task.medium == e_st_medium::slot)
    {
        FrontendService::show_statusbar(std::format("Loaded slot {}", st_slot + 1).c_str());
    }

    if (task.callback)
    {
        task.callback(Savestate::Result::Ok, decompressed_buf);
    }

failedLoad:
    extern bool ignore;
    //legacy .st fix, makes BEQ instruction ignore jump, because .st writes new address explictly.
    //This should cause issues anyway but libultra seems to be flexible (this means there's a chance it fails).
    //For safety, load .sts in dynarec because it completely avoids this issue by being differently coded
    g_st_old = (interp_addr == 0x80000180 || PC->addr == 0x80000180);
    //doubled because can't just reuse this variable
    if (interp_addr == 0x80000180 || (PC->addr == 0x80000180 && !dynacore))
        ignore = true;
    if (!dynacore && interpcore)
    {
        //g_core_logger->info(".st jump: {:#06x}, stopped here:{:#06x}", interp_addr, last_addr);
        last_addr = interp_addr;
    }
    else
    {
        //g_core_logger->info(".st jump: {:#06x}, stopped here:{:#06x}", PC->addr, last_addr);
        last_addr = PC->addr;
    }

    g_core_logger->info("Savestate loading took {}ms", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
}

/**
 * Logs the current task queue.
 */
void savestates_log_tasks()
{
    std::scoped_lock lock(g_task_mutex);
    g_core_logger->info("[ST] Begin task dump");
    for (const auto& task : g_st_tasks)
    {
        std::string job_str = (task.job == e_st_job::save) ? "Save" : "Load";
        std::string medium_str;
        switch (task.medium)
        {
        case e_st_medium::slot:
            medium_str = "Slot";
            break;
        case e_st_medium::path:
            medium_str = "Path";
            break;
        case e_st_medium::memory:
            medium_str = "Memory";
            break;
        default:
            medium_str = "Unknown";
            break;
        }
        g_core_logger->info("[ST] \tTask: Job = {}, Medium = {}", job_str, medium_str);
    }
    g_core_logger->info("[ST] End task dump");
}

/**
 * Warns if a savestate load task is scheduled after a save task.
 */
void savestates_warn_if_load_after_save()
{
    std::scoped_lock lock(g_task_mutex);

    bool encountered_load = false;
    for (const auto& task : g_st_tasks)
    {
        if (task.job == e_st_job::save && encountered_load)
        {
            g_core_logger->warn("[ST] A savestate save task is scheduled after a load task. This may cause unexpected behavior for the caller.");
            break;
        }

        if (task.job == e_st_job::load)
        {
            encountered_load = true;
        }
    }
}

void savestates_do_work()
{
    std::scoped_lock lock(g_task_mutex);

    if (!g_st_tasks.empty())
    {
        g_core_logger->info("[ST] Processing {} tasks...", g_st_tasks.size());
        savestates_log_tasks();
    }
    else
    {
        return;
    }

    for (const auto& task : g_st_tasks)
    {
        if (task.job == e_st_job::save)
        {
            savestates_save_immediate_impl(task);
        }
        else
        {
            savestates_load_immediate_impl(task);
        }
    }
    g_st_tasks.clear();
}


void savestates_do_file(const std::filesystem::path& path, const e_st_job job, const t_savestate_callback& callback)
{
    std::scoped_lock lock(g_task_mutex);
    g_st_tasks.insert(g_st_tasks.begin(), t_savestate_task{
                          .job = job,
                          .medium = e_st_medium::path,
                          .callback = callback,
                          .params = {
                              .path = path
                          }
                      });
    savestates_warn_if_load_after_save();
}

void savestates_do_slot(const int32_t slot, const e_st_job job, const t_savestate_callback& callback)
{
    std::scoped_lock lock(g_task_mutex);
    savestates_set_slot(slot == -1 ? st_slot : slot);

    g_st_tasks.insert(g_st_tasks.begin(), t_savestate_task{
                          .job = job,
                          .medium = e_st_medium::slot,
                          .callback = callback,
                          .params = {
                              .slot = (size_t)slot
                          }
                      });
    savestates_warn_if_load_after_save();
}

void savestates_do_memory(const std::vector<uint8_t>& buffer, const e_st_job job, const t_savestate_callback& callback)
{
    std::scoped_lock lock(g_task_mutex);
    g_st_tasks.insert(g_st_tasks.begin(), t_savestate_task{
                          .job = job,
                          .medium = e_st_medium::memory,
                          .callback = callback,
                          .params = {
                              .buffer = buffer
                          },
                      });
    savestates_warn_if_load_after_save();
}
