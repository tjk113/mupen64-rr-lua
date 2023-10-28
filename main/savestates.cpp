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

#include <zlib.h>
#include <libdeflate.h>
#include <stdlib.h>
#include <string.h>

#include "../lua/LuaConsole.h"
#include "vcr.h"

#include "savestates.h"
#include "guifuncs.h"
#include "rom.h"
#include "../memory/memory.h"
#include "../memory/flashram.h"
#include "../memory/summercart.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"
#include "win/main_win.h"
#include "win/features/Statusbar.hpp"

extern unsigned long interp_addr;

size_t st_slot = 0;
std::filesystem::path st_path;
e_st_job savestates_job = e_st_job::none;
int savestates_job_success = 1;
bool savestates_job_use_slot = false;

bool old_st; //.st that comes from no delay fix mupen, it has some differences compared to new st:
//- one frame of input is "embedded", that is the pif ram holds already fetched controller info.
//- execution continues at exception handler (after input poll) at 0x80000180.
bool lockNoStWarn; // I wont remove this variable for now, it should be used how the name suggests
// right now it's big mess and some other variable is used instead

bool fix_new_st = true; //this is a switch to enable/disable fixing .st to work for old mupen (and m64plus)
//read savestate save function for more info
//right now its hardcoded to enabled
bool st_skip_dma = false;
#define MUPEN64NEW_ST_FIXED (1<<31) //last bit seems to be free

std::filesystem::path get_effective_path()
{
	if (savestates_job_use_slot)
	{
		return std::format("{}{}.st{}", get_savespath(), (const char*)ROM_HEADER->nom, std::to_string(st_slot));
	}
	return st_path;
}

void savestates_save_immediate()
{
	auto start_time = std::chrono::high_resolution_clock::now();
    savestates_job_success = TRUE;

	std::filesystem::path path = get_effective_path();
    if (Config.use_summercart) save_summercart(path.string().c_str());

	std::vector<uint8_t> b;

    vecwrite(b, ROM_SETTINGS.MD5, 32);

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
        for (size_t i = 0; i < (64 / 4); i++)
            rdram[si_register.si_dram_addr / 4 + i] = sl(PIF_RAM[i]);
        update_count();
        add_interupt_event(SI_INT, /*0x100*/0x900);
        rdram_register.rdram_device_manuf |= MUPEN64NEW_ST_FIXED;
        st_skip_dma = true;
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

    vecwrite(b, &next_interupt, 4);
    vecwrite(b, &next_vi, 4);
    vecwrite(b, &vi_field, 4);

    int len = save_eventqueue_infos(buf);
    vecwrite(b, buf, len);

    // re-recording
    BOOL movieActive = VCR_isActive();
    vecwrite(b, &movieActive, sizeof(movieActive));
    if (movieActive)
    {
        char* movie_freeze_buf = NULL;
        unsigned long movie_freeze_size = 0;

        VCR_movieFreeze(&movie_freeze_buf, &movie_freeze_size);
        if (movie_freeze_buf)
        {
            vecwrite(b, &movie_freeze_size, sizeof(movie_freeze_size));
            vecwrite(b, movie_freeze_buf, movie_freeze_size);
            free(movie_freeze_buf);
        }
        else
        {
            printf("Failed to save movie snapshot.\n");
            savestates_job_success = FALSE;
        }
    }

	std::vector<uint8_t> compressed = b;
	auto compressor = libdeflate_alloc_compressor(6);
	size_t final_size = libdeflate_gzip_compress(compressor, b.data(), b.size(), compressed.data(), compressed.size());
	compressed.resize(final_size);

	FILE* f = fopen(path.string().c_str(), "wb");
	fwrite(compressed.data(), compressed.size(), 1, f);
	fclose(f);

    main_dispatcher_invoke(AtSaveStateLuaCallback);
	statusbar_send_text(std::format("Saved {}", path.filename().string()));
	printf("Savestate saving took %dms\n", (std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000);

}

// reads memory like a file
void memread(char** src, void* dest, unsigned int len)
{
    memcpy(dest, *src, len);
    *src += len;
}

/// <summary>
/// overwrites emu memory with given data. Make sure to call load_eventqueue_infos as well!!!
/// </summary>
/// <param name="firstBlock"></param>
void load_memory_from_buffer(char* p)
{
    memread(&p, &rdram_register, sizeof(RDRAM_register));
    if (rdram_register.rdram_device_manuf & MUPEN64NEW_ST_FIXED)
    {
        rdram_register.rdram_device_manuf &= ~MUPEN64NEW_ST_FIXED; //remove the trick
        st_skip_dma = true; //tell dma.c to skip it
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
        for (int i = 0; i < 0x100000; i++)
            invalid_code[i] = 1;
        jump_to(target_addr)
    }

    memread(&p, &next_interupt, 4);
    memread(&p, &next_vi, 4);
    memread(&p, &vi_field, 4);
}

/// <summary>
/// First decompresses file into buffer, then before overwriting emulator memory checks if its good
/// </summary>
/// <param name="silence_not_found_error"></param>
void savestates_load_immediate()
{
    /*rough .st format :
    0x0 - 0xA02BB0 : memory, registers, stuff like that, known size
    0xA02BB4 - ??? : interrupt queue, dynamic size (cap 1kB)
    ??? - ??????   : m64 info, also dynamic, no cap
    More precise info can be seen on github
    */
    constexpr int BUFLEN = 1024;
    constexpr int firstBlockSize = 0xA02BB4 - 32; //32 is md5 hash
    char buf[BUFLEN];
    //handle to st
    int len;

    savestates_job_success = TRUE;

    std::filesystem::path path = get_effective_path();
    gzFile f = gzopen(path.string().c_str(), "rb");

    //failed opening st
    if (f == nullptr)
    {
    	statusbar_send_text(std::format("{} not found", path.filename().string()));
        savestates_job_success = FALSE;
        return;
    }
    if (Config.use_summercart) load_summercart(path.string().c_str());


    // compare current rom hash with one stored in state
    gzread(f, buf, 32);
    if (memcmp(buf, ROM_SETTINGS.MD5, 32) != 0)
    {
        if (Config.is_rom_movie_compatibility_check_enabled) // if true, allows loading
            warn_savestate("Savestate Warning",
                           "You have option 'Allow loading movies on wrong roms' selected.\nMismatched .st is going to be loaded",
                           TRUE);
        else
        {
            warn_savestate("Wrong ROM error", "This savestate is from another ROM or version.", TRUE);
            gzclose(f);
            savestates_job_success = FALSE;
            return;
        }
    }

    // new version does one bigass gzread for first part of .st (static size)
    char* firstBlock = (char*)malloc(firstBlockSize);
    gzread(f, firstBlock, firstBlockSize);
    // now read interrupt queue into buf
    for (len = 0; len < BUFLEN; len += 8)
    {
        gzread(f, buf + len, 4);
        if (*reinterpret_cast<unsigned long*>(&buf[len]) == 0xFFFFFFFF)
            break;
        gzread(f, buf + len + 4, 4);
    }
    if (len == BUFLEN)
    {
        // Exhausted the buffer and still no terminator. Prevents the buffer overflow "Queuecrush".
        fprintf(stderr, "Snapshot event queue terminator not reached.\n");
        savestates_job_success = FALSE;
        warn_savestate("Savestate error", "Event queue too long (Savestate corrupted?)");
        gzclose(f);
        free(firstBlock);
        savestates_job_success = FALSE;
        return;
    }
    // this will be later
    // load_eventqueue_infos(buf);

    // now read movie info if exists
    uint32_t isMovie;
    gzread(f, &isMovie, sizeof(isMovie));

    if (!isMovie) // this .st is not part of a movie
    {
        if (VCR_isActive())
        {
            if (!Config.is_state_independent_state_loading_allowed)
            {
                fprintf(stderr, "Can't load a non-movie snapshot while a movie is active.\n");
                warn_savestate("Savestate error", "Can't load a non-movie snapshot while a movie is active.\n");
                gzclose(f);
                free(firstBlock);
                savestates_job_success = FALSE;
                return;
            }
            else
            {
                statusbar_send_text("Warning: loading non-movie savestate can desync playback");
            }
        }

        // at this point we know the savestate is safe to be loaded (done after else block)
    }
    else
    {
        // this .st is part of a movie
        // rest of the file can be gzreaded now

        // hash matches, load and verify rest of the data
        unsigned long movieInputDataSize = 0;
        gzread(f, &movieInputDataSize, sizeof(movieInputDataSize));
        char* local_movie_data = (char*)malloc(movieInputDataSize * sizeof(char));
        if (!local_movie_data)
        {
            fprintf(stderr, "Out of memory while loading .st\n");
            savestates_job_success = FALSE;
            goto failedLoad;
        }
        int readBytes = gzread(f, local_movie_data, movieInputDataSize);
        if ((unsigned long)readBytes != movieInputDataSize)
        {
            fprintf(stderr, "Error while loading .st, file was too short.\n");
            free(local_movie_data);
            savestates_job_success = FALSE;
            goto failedLoad;
        }
        int code = VCR_movieUnfreeze(local_movie_data, movieInputDataSize);
        free(local_movie_data);
        if (code != SUCCESS && !VCR_isIdle())
        {
            bool stop = false;
            char errStr[1024];
            strcpy(errStr, "Failed to load movie snapshot,\n");
            switch (code)
            {
            case NOT_FROM_THIS_MOVIE:
                strcat(errStr, "snapshot not from this movie\n");
                break;
            case NOT_FROM_A_MOVIE:
                strcat(errStr, "not a movie snapshot\n");
                break; // shouldn't get here...
            case INVALID_FRAME:
                strcat(errStr, "invalid frame number\n");
                break;
            case WRONG_FORMAT:
                strcat(errStr, "wrong format\n");
                stop = true;
                break;
            default:
                break;
            }
            if (!Config.is_state_independent_state_loading_allowed)
            {
                printWarning(errStr);
                if (stop && VCR_isRecording()) VCR_stopRecord(1);
                else if (stop) VCR_stopPlayback();
                savestates_job_success = FALSE;
                goto failedLoad;
            }
            else
            {
                statusbar_send_text("Warning: loading non-movie savestate can break recording");
            }
        }
    }

    // so far loading success! overwrite memory
    load_eventqueue_infos(buf);
    load_memory_from_buffer(firstBlock);
    free(firstBlock);
    main_dispatcher_invoke(AtLoadStateLuaCallback);
	statusbar_send_text(std::format("Loaded {}", path.filename().string()));
failedLoad:

    gzclose(f);
    extern bool ignore;
    //legacy .st fix, makes BEQ instruction ignore jump, because .st writes new address explictly.
    //This should cause issues anyway but libultra seems to be flexible (this means there's a chance it fails).
    //For safety, load .sts in dynarec because it completely avoids this issue by being differently coded
    old_st = (interp_addr == 0x80000180 || PC->addr == 0x80000180);
    //doubled because can't just reuse this variable
    if (interp_addr == 0x80000180 || (PC->addr == 0x80000180 && !dynacore))
        ignore = true;
    if (!dynacore && interpcore)
    {
        //printf(".st jump: %x, stopped here:%x\n", interp_addr, last_addr);
        last_addr = interp_addr;
    }
    else
    {
        //printf(".st jump: %x, stopped here:%x\n", PC->addr, last_addr);
        last_addr = PC->addr;
    }
}

void savestates_do(std::filesystem::path path, e_st_job job)
{
	st_path = path;
	savestates_job = job;
	savestates_job_use_slot = false;
}

void savestates_do(size_t slot, e_st_job job)
{
	st_slot = slot;
	savestates_job = job;
	savestates_job_use_slot = true;
}

#undef BUFLEN
