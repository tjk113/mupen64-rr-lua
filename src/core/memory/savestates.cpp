/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "savestates.h"
#include <libdeflate.h>
#include <core/Core.h>
#include <core/r4300/interrupt.h>
#include <core/r4300/r4300.h>
#include <core/r4300/rom.h>
#include <core/r4300/vcr.h>
#include <core/include/core_api.h>
#include <StlExtensions.h>
#include <IOHelpers.h>
#include "flashram.h"
#include "memory.h"
#include "summercart.h"

// st that comes from no delay fix mupen, it has some differences compared to new st:
// - one frame of input is "embedded", that is the pif ram holds already fetched controller info.
// - execution continues at exception handler (after input poll) at 0x80000180.
bool g_st_old;

// read savestate save function for more info
// right now its hardcoded to enabled
bool g_st_skip_dma = false;

/// Represents a task to be performed by the savestate system.
struct t_savestate_task {
    struct t_params {
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
    Job job;

    /// The savestate's source or target medium.
    Medium medium;

    /// Callback to invoke when the task finishes. Mustn't be null.
    t_savestate_callback callback;

    /// The task's parameters. Only one field in the struct is valid at a time.
    t_params params{};

    /// Whether warnings, such as those about ROM compatibility, shouldn't be shown.
    bool ignore_warnings;
};

// Enable fixing .st to work for old mupen and m64p
constexpr bool FIX_NEW_ST = true;

// Bit to set in RDRAM register to indicate that the savestate has been fixed
constexpr auto RDRAM_DEVICE_MANUF_NEW_FIX_BIT = (1 << 31);

// The task vector mutex. Locked when accessing the task vector.
std::recursive_mutex g_task_mutex;

// The task vector, which contains the task queue to be performed by the savestate system.
std::vector<t_savestate_task> g_tasks;

// Demarcator for new screenshot section
char screen_section[] = "SCR";

// Buffer used for storing flashram data during loading
char g_flashram_buf[1024]{};

// Buffer used for storing event queue data during loading
char g_event_queue_buf[1024]{};

// Buffer used for storing st data up to event queue
uint8_t g_first_block[0xA02BB4 - 32]{};

// The undo savestate buffer.
std::vector<uint8_t> g_undo_savestate;

void get_paths_for_task(const t_savestate_task& task, std::filesystem::path& st_path, std::filesystem::path& sd_path)
{
    sd_path = std::format("{}{}.sd", g_core->get_saves_directory().string(), (const char*)ROM_HEADER.nom);

    if (task.medium == Medium::Slot)
    {
        st_path = std::format(
        L"{}{} {}.st{}",
        g_core->get_saves_directory().wstring(),
        string_to_wstring((const char*)ROM_HEADER.nom),
        core_vr_country_code_to_country_name(ROM_HEADER.Country_code), std::to_wstring(task.params.slot));
    }
}


void load_memory_from_buffer(uint8_t* p)
{
    memread(&p, &rdram_register, sizeof(RDRAM_register));
    if (rdram_register.rdram_device_manuf & RDRAM_DEVICE_MANUF_NEW_FIX_BIT)
    {
        rdram_register.rdram_device_manuf &= ~RDRAM_DEVICE_MANUF_NEW_FIX_BIT; // remove the trick
        g_st_skip_dma = true; // tell dma.c to skip it
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
    for (int32_t i = 0; i < 32; i++)
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

    b.reserve(0xB624F0);

    memset(g_flashram_buf, 0, sizeof(g_flashram_buf));
    memset(g_event_queue_buf, 0, sizeof(g_event_queue_buf));

    t_movie_freeze freeze{};
    uint32_t movie_active = core_vcr_freeze(&freeze);

    if (FIX_NEW_ST)
    {
        // this is code taken from dma.c:dma_si_read(), it finishes up the dma.
        // it copies data from pif (should contain commands and controller states), updates count reg and adds SI interrupt to queue
        // so why does old mupen and mupen64plus dislike savestates without doing this? well in case of mario 64 it leaves pif command buffer uninitialised
        // and it never can poll input properly (hence the inability to frame advance which is handled inside controller read).

        // But we dont want to do this then load such .st and dma again... so I notify mupen about this in .st,
        // since .st is filled up to the brim with data (not even a single unused offset) I have to store one bit in... rdram manufacturer register
        // this 99.999% wont break on any game, and that bit will be cleared by mupen64plus converter as well, so only old old mupen ever sees this trick.

        // update: I stumbled upon a .st that had the bit set, but didn't have SI_INT in queue,
        // so it froze game, so there exists a way to cause that somehow
        if (get_event(SI_INT) == 0) // if there is no interrupt, add it, otherwise dont care
        {
            g_core->logger->warn("[ST] No SI interrupt in queue, adding one...");
            for (size_t i = 0; i < (64 / 4); i++)
                rdram[si_register.si_dram_addr / 4 + i] = sl(PIF_RAM[i]);
            update_count();
            add_interrupt_event(SI_INT, /*0x100*/ 0x900);
            rdram_register.rdram_device_manuf |= RDRAM_DEVICE_MANUF_NEW_FIX_BIT;
            g_st_skip_dma = true;
        }
        // hack end
    }

    // NOTE: This saving needs to be done **after** the fixing block, as it is now. See previous regression in f9d58f639c798cbc26bbb808b1c3dbd834ffe2d9.
    save_flashram_infos(g_flashram_buf);
    const int32_t event_queue_len = save_eventqueue_infos(g_event_queue_buf);

    vecwrite(b, rom_md5, 32);
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
    vecwrite(b, g_flashram_buf, 24);
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
    vecwrite(b, g_event_queue_buf, event_queue_len);
    vecwrite(b, &movie_active, sizeof(movie_active));
    if (movie_active)
    {
        vecwrite(b, &freeze.size, sizeof(freeze.size));
        vecwrite(b, &freeze.uid, sizeof(freeze.uid));
        vecwrite(b, &freeze.current_sample, sizeof(freeze.current_sample));
        vecwrite(b, &freeze.current_vi, sizeof(freeze.current_vi));
        vecwrite(b, &freeze.length_samples, sizeof(freeze.length_samples));
        vecwrite(b, freeze.input_buffer.data(), freeze.input_buffer.size() * sizeof(BUTTONS));
    }

    if (core_is_mge_available() && g_core->cfg->st_screenshot)
    {
        int32_t width;
        int32_t height;
        g_core->plugin_funcs.get_video_size(&width, &height);

        void* video = nullptr;
        g_core->plugin_funcs.read_video(&video);

        vecwrite(b, screen_section, sizeof(screen_section));
        vecwrite(b, &width, sizeof(width));
        vecwrite(b, &height, sizeof(height));
        vecwrite(b, video, width * height * 3);

        free(video);
    }

    return b;
}

void savestates_save_immediate_impl(const t_savestate_task& task)
{
    ScopeTimer timer("Savestate saving", g_core->logger);

    const auto st = generate_savestate();

    if (task.medium == Medium::Slot || task.medium == Medium::Path)
    {
        // Always save summercart for some reason
        std::filesystem::path new_st_path = task.params.path;
        std::filesystem::path new_sd_path = "";
        get_paths_for_task(task, new_st_path, new_sd_path);
        if (g_core->cfg->use_summercart)
            save_summercart(new_sd_path.string().c_str());

        // Generate compressed buffer
        std::vector<uint8_t> compressed_buffer;
        compressed_buffer.resize(st.size());

        const auto compressor = libdeflate_alloc_compressor(6);
        const size_t final_size = libdeflate_gzip_compress(compressor, st.data(), st.size(), compressed_buffer.data(), compressed_buffer.size());
        libdeflate_free_compressor(compressor);
        compressed_buffer.resize(final_size);

        // write compressed st to disk
        FILE* f = fopen(new_st_path.string().c_str(), "wb");

        if (f == nullptr)
        {
            task.callback(ST_FileWriteError, st);
            return;
        }

        fwrite(compressed_buffer.data(), compressed_buffer.size(), 1, f);
        fclose(f);
    }

    task.callback(Res_Ok, st);
    g_core->callbacks.save_state();
}

void savestates_load_immediate_impl(const t_savestate_task& task)
{
    ScopeTimer timer("Savestate loading", g_core->logger);

    memset(g_event_queue_buf, 0, sizeof(g_event_queue_buf));

    std::filesystem::path new_st_path = task.params.path;
    std::filesystem::path new_sd_path = "";
    get_paths_for_task(task, new_st_path, new_sd_path);

    if (g_core->cfg->use_summercart)
        load_summercart(new_sd_path.string().c_str());

    std::vector<uint8_t> st_buf;

    switch (task.medium)
    {
    case Medium::Slot:
    case Medium::Path:
        st_buf = read_file_buffer(new_st_path);
        break;
    case Medium::Memory:
        st_buf = task.params.buffer;
        break;
    default:
        assert(false);
    }

    if (st_buf.empty())
    {
        task.callback(ST_NotFound, {});
        return;
    }

    std::vector<uint8_t> decompressed_buf = auto_decompress(st_buf);
    if (decompressed_buf.empty())
    {
        task.callback(ST_DecompressionError, {});
        return;
    }

    // BUG (PRONE): we arent allowed to hold on to a vector element pointer
    // find another way of doing this
    auto ptr = decompressed_buf.data();

    // compare current rom hash with one stored in state
    char md5[33] = {0};
    memread(&ptr, &md5, 32);

    if (!task.ignore_warnings && memcmp(md5, rom_md5, 32))
    {
        auto result = g_core->show_ask_dialog(std::format(
                                              L"The savestate was created on a rom with hash {}, but is being loaded on another rom.\r\nThe emulator may crash. Are you sure you want to continue?",
                                              string_to_wstring(md5))
                                              .c_str(),
                                              L"Savestate", true);

        if (!result)
        {
            task.callback(ST_Cancelled, {});
            return;
        }
    }

    // new version does one bigass gzread for first part of .st (static size)
    memread(&ptr, g_first_block, sizeof(g_first_block));

    // now read interrupt queue into buf
    int32_t len;
    for (len = 0; len < sizeof(g_event_queue_buf); len += 8)
    {
        memread(&ptr, g_event_queue_buf + len, 4);
        if (*reinterpret_cast<uint32_t*>(&g_event_queue_buf[len]) == 0xFFFFFFFF)
            break;
        memread(&ptr, g_event_queue_buf + len + 4, 4);
    }
    if (len == sizeof(g_event_queue_buf))
    {
        // Exhausted the buffer and still no terminator. Prevents the buffer overflow "Queuecrush".
        task.callback(ST_EventQueueTooLong, {});
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

        const auto code = core_vcr_unfreeze(freeze);

        if (!task.ignore_warnings && code != Res_Ok && core_vcr_get_task() != task_idle)
        {
            std::wstring err_str = L"Failed to restore movie, ";
            switch (code)
            {
            case VCR_NotFromThisMovie:
                err_str += L"the savestate is not from this movie.";
                break;
            case VCR_InvalidFrame:
                err_str += L"the savestate frame is outside the bounds of the movie.";
                break;
            case VCR_InvalidFormat:
                err_str += L"the savestate freeze buffer format is invalid.";
                break;
            default:
                err_str += L"an unknown error has occured.";
                break;
            }
            err_str += L" Loading the savestate might desynchronize the movie.\r\nAre you sure you want to continue?";

            const auto result = g_core->show_ask_dialog(err_str.c_str(), L"Savestate", true);
            if (!result)
            {
                task.callback(ST_Cancelled, {});
                goto failedLoad;
            }
        }
    }
    else
    {
        if (!task.ignore_warnings && (core_vcr_get_task() == task_recording || core_vcr_get_task() == task_playback))
        {
            const auto result = g_core->show_ask_dialog(
            L"The savestate is not from a movie. Loading it might desynchronize the movie.\r\nAre you sure you want to continue?", L"Savestate", true);
            if (!result)
            {
                task.callback(ST_Cancelled, {});
                return;
            }
        }

        // at this point we know the savestate is safe to be loaded (done after else block)
    }
    {
        g_core->logger->info("[Savestates] {} bytes remaining", decompressed_buf.size() - (ptr - decompressed_buf.data()));
        int32_t video_width = 0;
        int32_t video_height = 0;
        void* video_buffer = nullptr;
        if (decompressed_buf.size() - (ptr - decompressed_buf.data()) > 0)
        {
            char scr_section[sizeof(screen_section)] = {0};
            memread(&ptr, scr_section, sizeof(screen_section));

            if (!memcmp(scr_section, screen_section, sizeof(screen_section)))
            {
                g_core->logger->info("[Savestates] Restoring screen buffer...");
                memread(&ptr, &video_width, sizeof(video_width));
                memread(&ptr, &video_height, sizeof(video_height));

                video_buffer = malloc(video_width * video_height * 3);
                memread(&ptr, video_buffer, video_width * video_height * 3);
            }
        }

        // so far loading success! overwrite memory
        load_eventqueue_infos(g_event_queue_buf);
        load_memory_from_buffer(g_first_block);

        // NOTE: We don't want to restore screen buffer while seeking, since it creates a int16_t ugly flicker when the movie restarts by loading state
        if (core_is_mge_available() && video_buffer && !core_vcr_is_seeking())
        {
            int32_t current_width, current_height;
            g_core->plugin_funcs.get_video_size(&current_width, &current_height);
            if (current_width == video_width && current_height == video_height)
            {
                g_core->load_screen(video_buffer);
                free(video_buffer);
            }
        }
    }

    g_core->callbacks.load_state();
    task.callback(Res_Ok, decompressed_buf);

failedLoad:
    // legacy .st fix, makes BEQ instruction ignore jump, because .st writes new address explictly.
    // This should cause issues anyway but libultra seems to be flexible (this means there's a chance it fails).
    // For safety, load .sts in dynarec because it completely avoids this issue by being differently coded
    g_st_old = (interp_addr == 0x80000180 || PC->addr == 0x80000180);
    // doubled because can't just reuse this variable
    if (interp_addr == 0x80000180 || (PC->addr == 0x80000180 && !dynacore))
        g_vr_beq_ignore_jmp = true;
    if (!dynacore && interpcore)
    {
        // g_core->logger->info(".st jump: {:#06x}, stopped here:{:#06x}", interp_addr, last_addr);
        last_addr = interp_addr;
    }
    else
    {
        // g_core->logger->info(".st jump: {:#06x}, stopped here:{:#06x}", PC->addr, last_addr);
        last_addr = PC->addr;
    }
}

/**
 * Simplifies the task queue by removing duplicates. Only slot-based tasks are affected for now.
 */
void savestates_simplify_tasks()
{
    std::scoped_lock lock(g_task_mutex);
    g_core->logger->info("[ST] Simplifying task queue...");

    std::vector<size_t> duplicate_indicies{};


    // De-dup slot-based save tasks
    // 1. Loop through all tasks
    for (size_t i = 0; i < g_tasks.size(); i++)
    {
        const auto& task = g_tasks[i];

        if (task.medium != Medium::Slot)
            continue;

        // 2. If a slot task is detected, loop through all other tasks up to the next load task to find duplicates
        for (size_t j = i + 1; j < g_tasks.size(); j++)
        {
            const auto& other_task = g_tasks[j];

            if (other_task.job == Job::Load)
            {
                break;
            }

            if (other_task.medium == Medium::Slot && task.params.slot == other_task.params.slot)
            {
                g_core->logger->info("[ST] Found duplicate slot task at index {}", j);
                duplicate_indicies.push_back(j);
            }
        }
    }

    g_tasks = erase_indices(g_tasks, duplicate_indicies);
}

/**
 * Warns if a savestate load task is scheduled after a save task.
 */
void savestates_warn_if_load_after_save()
{
    std::scoped_lock lock(g_task_mutex);

    bool encountered_load = false;
    for (const auto& task : g_tasks)
    {
        if (task.job == Job::Save && encountered_load)
        {
            g_core->logger->warn("[ST] A savestate save task is scheduled after a load task. This may cause unexpected behavior for the caller.");
            break;
        }

        if (task.job == Job::Load)
        {
            encountered_load = true;
        }
    }
}

/**
 * Logs the current task queue.
 */
void savestates_log_tasks()
{
    std::scoped_lock lock(g_task_mutex);
    g_core->logger->info("[ST] Begin task dump");
    savestates_warn_if_load_after_save();
    for (const auto& task : g_tasks)
    {
        std::string job_str = (task.job == Job::Save) ? "Save" : "Load";
        std::string medium_str;
        switch (task.medium)
        {
        case Medium::Slot:
            medium_str = "Slot";
            break;
        case Medium::Path:
            medium_str = "Path";
            break;
        case Medium::Memory:
            medium_str = "Memory";
            break;
        default:
            medium_str = "Unknown";
            break;
        }
        g_core->logger->info("[ST] \tTask: Job = {}, Medium = {}", job_str, medium_str);
    }
    g_core->logger->info("[ST] End task dump");
}

/**
 * Inserts a save operation at the start of the queue (whose callback assigns the undo savestate buffer) if the task queue contains one or more load operations.
 */
void savestates_create_undo_point()
{
    if (!g_core->cfg->st_undo_load)
    {
        return;
    }

    bool queue_contains_load = std::ranges::any_of(g_tasks, [](const t_savestate_task& task) {
        return task.job == Job::Load;
    });

    if (!queue_contains_load)
    {
        g_core->logger->trace("[ST] Skipping undo point creation: no load in queue.");
        return;
    }

    g_core->logger->trace("[ST] Inserting undo point creation into task queue...");

    const t_savestate_task task = {
    .job = Job::Save,
    .medium = Medium::Memory,
    .callback = [](const core_result result, const std::vector<uint8_t>& buffer) {
        if (result != Res_Ok)
        {
            return;
        }

        std::scoped_lock lock(g_task_mutex);
        g_undo_savestate = buffer;
    },
    .params = {
    .buffer = {},
    },
    .ignore_warnings = true,
    };

    g_tasks.insert(g_tasks.begin(), task);
}

void st_do_work()
{
    std::scoped_lock lock(g_task_mutex);

    if (g_tasks.empty())
    {
        return;
    }

    savestates_simplify_tasks();
    savestates_create_undo_point();
    savestates_simplify_tasks();
    savestates_log_tasks();

    for (const auto& task : g_tasks)
    {
        if (task.job == Job::Save)
        {
            savestates_save_immediate_impl(task);
        }
        else
        {
            savestates_load_immediate_impl(task);
        }
    }
    g_tasks.clear();
}

void st_on_core_stop()
{
    std::scoped_lock lock(g_task_mutex);
    g_tasks.clear();
    g_undo_savestate.clear();
}

/**
 * Gets whether work can currently be enqueued.
 */
bool can_push_work()
{
    return core_executing;
}

bool core_st_do_file(const std::filesystem::path& path, const Job job, const t_savestate_callback& callback, bool ignore_warnings)
{
    std::scoped_lock lock(g_task_mutex);

    if (!can_push_work())
    {
        g_core->logger->trace("[ST] do_file: Can't enqueue work.");
        if (callback)
        {
            callback(ST_CoreNotLaunched, {});
        }
        return false;
    }

    auto pre_callback = [=](const core_result result, const std::vector<uint8_t>& buffer) {
        if (result == Res_Ok)
        {
            g_core->show_statusbar(std::format(L"{} {}", job == Job::Save ? L"Saved" : L"Loaded", path.filename().wstring()).c_str());
        }
        else if (result == ST_Cancelled)
        {
            g_core->show_statusbar(std::format(L"Cancelled {}", job == Job::Save ? L"save" : L"load").c_str());
        }
        else
        {
            const auto message = std::format(L"Failed to {} {} (error code {}).\nVerify that the savestate is valid and accessible.",
                                             job == Job::Save ? L"save" : L"load", path.filename().wstring(), (int32_t)result);
            g_core->show_dialog(message.c_str(), L"Savestate", fsvc_error);
        }

        if (callback)
        {
            callback(result, buffer);
        }
    };

    const t_savestate_task task = {
    .job = job,
    .medium = Medium::Path,
    .callback = pre_callback,
    .params = {
    .path = path},
    .ignore_warnings = ignore_warnings,
    };

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}

bool core_st_do_slot(const int32_t slot, const Job job, const t_savestate_callback& callback, bool ignore_warnings)
{
    std::scoped_lock lock(g_task_mutex);

    if (!can_push_work())
    {
        g_core->logger->trace("[ST] do_slot: Can't enqueue work.");
        if (callback)
        {
            callback(ST_CoreNotLaunched, {});
        }
        return false;
    }

    auto pre_callback = [=](const core_result result, const std::vector<uint8_t>& buffer) {
        if (result == Res_Ok)
        {
            g_core->show_statusbar(std::format(L"{} slot {}", job == Job::Save ? L"Saved" : L"Loaded", slot + 1).c_str());
        }
        else if (result == ST_Cancelled)
        {
            g_core->show_statusbar(std::format(L"Cancelled {}", job == Job::Save ? L"save" : L"load").c_str());
        }
        else
        {
            g_core->show_statusbar(std::format(L"Failed to {} slot {}", job == Job::Save ? L"save" : L"load", slot + 1).c_str());
        }

        if (callback)
        {
            callback(result, buffer);
        }
    };

    const t_savestate_task task = {
    .job = job,
    .medium = Medium::Slot,
    .callback = pre_callback,
    .params = {
    .slot = static_cast<size_t>(slot)},
    .ignore_warnings = ignore_warnings,
    };

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}

bool core_st_do_memory(const std::vector<uint8_t>& buffer, const Job job, const t_savestate_callback& callback, bool ignore_warnings)
{
    std::scoped_lock lock(g_task_mutex);

    if (!can_push_work())
    {
        g_core->logger->trace("[ST] do_memory: Can't enqueue work.");
        if (callback)
        {
            callback(ST_CoreNotLaunched, {});
        }
        return false;
    }

    const t_savestate_task task = {
    .job = job,
    .medium = Medium::Memory,
    .callback = callback,
    .params = {
    .buffer = buffer},
    .ignore_warnings = ignore_warnings,
    };

    g_tasks.insert(g_tasks.begin(), task);
    return true;
}

void core_st_get_undo_savestate(std::vector<uint8_t>& buffer)
{
    std::scoped_lock lock(g_task_mutex);
    buffer.clear();
    buffer = g_undo_savestate;
}


void core_vr_st_wait_increment()
{
    ++g_vr_wait_before_input_poll;
}

void core_vr_st_wait_decrement()
{
    --g_vr_wait_before_input_poll;
}
