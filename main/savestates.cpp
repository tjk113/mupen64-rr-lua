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
#include "guifuncs.h"
#include "LuaCallbacks.h"
#include "messenger.h"
#include "../r4300/rom.h"
#include "vcr.h"
#include "../lua/LuaConsole.h"
#include "../memory/flashram.h"
#include "../memory/memory.h"
#include "../memory/summercart.h"
#include "../r4300/interrupt.h"
#include "../r4300/r4300.h"
#include "win/Config.hpp"
#include "win/main_win.h"
#include "win/features/Statusbar.hpp"

std::unordered_map<std::string, std::vector<uint8_t>> st_buffers;
size_t st_slot = 0;
std::string st_key;
std::filesystem::path st_path;
e_st_job savestates_job = e_st_job::none;
e_st_medium st_medium = e_st_medium::path;
bool savestates_job_success = true;

// st that comes from no delay fix mupen, it has some differences compared to new st:
// - one frame of input is "embedded", that is the pif ram holds already fetched controller info.
// - execution continues at exception handler (after input poll) at 0x80000180.
bool old_st;

// enable fixing .st to work for old mupen (and m64plus)
bool fix_new_st = true;

// read savestate save function for more info
// right now its hardcoded to enabled
bool st_skip_dma = false;

//last bit seems to be free
enum { new_st_fixed_bit = (1<<31) };

constexpr int buflen = 1024;
constexpr int first_block_size = 0xA02BB4 - 32; //32 is md5 hash
uint8_t first_block[first_block_size] = {0};


std::filesystem::path get_saves_directory()
{
	if (Config.is_default_saves_directory_used)
	{
		return app_path + "Save\\";
	}
	return Config.saves_directory;
}

std::filesystem::path get_sram_path()
{
	return std::format("{}{}.sra", get_saves_directory().string(), (const char*)ROM_HEADER.nom);
}

std::filesystem::path get_eeprom_path()
{
	return std::format("{}{}.eep", get_saves_directory().string(), (const char*)ROM_HEADER.nom);
}

std::filesystem::path get_flashram_path()
{
	return std::format("{}{}.fla", get_saves_directory().string(), (const char*)ROM_HEADER.nom);
}

std::filesystem::path get_mempak_path()
{
	return std::format("{}{}.mpk", get_saves_directory().string(), (const char*)ROM_HEADER.nom);
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
			st_skip_dma = true;
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
    BOOL movie_active = vcr_is_active();
    vecwrite(b, &movie_active, sizeof(movie_active));
    if (movie_active)
    {
        char* movie_freeze_buf = nullptr;
        unsigned long movie_freeze_size = 0;

        vcr_movie_freeze(&movie_freeze_buf, &movie_freeze_size);
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
	return b;
}

void get_effective_paths(std::filesystem::path& st_path, std::filesystem::path& sd_path)
{
	sd_path = std::format("{}{}.sd", get_saves_directory().string(), (const char*)ROM_HEADER.nom);

	if (st_medium == e_st_medium::slot)
	{
		st_path = std::format("{}{}.st{}", get_saves_directory().string(), (const char*)ROM_HEADER.nom, std::to_string(st_slot));
	}
}

void savestates_save_immediate()
{

	const auto start_time = std::chrono::high_resolution_clock::now();
    savestates_job_success = TRUE;

	const auto st = generate_savestate();

	if (!savestates_job_success)
	{
		Statusbar::post("Failed to save savestate");
		savestates_job_success = FALSE;
		return;
	}

	if (st_medium == e_st_medium::slot && Config.increment_slot)
	{
		if (st_slot >= 9)
		{
			savestates_set_slot(0);
		} else
		{
			savestates_set_slot(st_slot + 1);
		}
 	}

	if (st_medium == e_st_medium::slot || st_medium == e_st_medium::path)
	{
		// Always save summercart for some reason
		std::filesystem::path new_st_path = st_path;
		std::filesystem::path new_sd_path = "";
		get_effective_paths(new_st_path, new_sd_path);
		if (Config.use_summercart) save_summercart(new_sd_path.string().c_str());

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
			Statusbar::post("Failed to save savestate");
			savestates_job_success = FALSE;
			return;
		}

		fwrite(compressed_buffer.data(), compressed_buffer.size(), 1, f);
		fclose(f);

		if (st_medium == e_st_medium::path)
		{
			Statusbar::post(std::format("Saved {}", new_st_path.filename().string()));
		} else
		{
			Statusbar::post(std::format("Saved slot {}", st_slot + 1));
		}
	} else
	{
		// Keep uncompressed buffer in memory
		st_buffers[st_key] = st;
	}

	LuaCallbacks::call_save_state();
	printf("Savestate saving took %dms\n", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
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
        for (char& i : invalid_code)
	        i = 1;
        jump_to(target_addr)
    }

    memread(&p, &next_interrupt, 4);
    memread(&p, &next_vi, 4);
    memread(&p, &vi_field, 4);
}

/// <summary>
/// First decompresses file into buffer, then before overwriting emulator memory checks if its good
/// </summary>
/// <param name="silence_not_found_error"></param>
void savestates_load_immediate()
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

    savestates_job_success = TRUE;

	std::filesystem::path new_st_path = st_path;
	std::filesystem::path new_sd_path = "";
	get_effective_paths(new_st_path, new_sd_path);

	if (Config.use_summercart) load_summercart(new_sd_path.string().c_str());

    std::vector<uint8_t> st_buf;

	switch (st_medium)
	{
	case e_st_medium::slot:
	case e_st_medium::path:
		st_buf = read_file_buffer(new_st_path);
		break;
	case e_st_medium::memory:
		if (st_buffers.contains(st_key))
		{
			st_buf = st_buffers[st_key];
		}
		break;
		default: assert(false);
	}

    if (st_buf.empty())
    {
    	Statusbar::post(std::format("{} not found", new_st_path.filename().string()));
        savestates_job_success = FALSE;
        return;
    }

	std::vector<uint8_t> decompressed_buf = auto_decompress(st_buf);
	if(decompressed_buf.empty())
	{
		MessageBox(mainHWND, "Failed to decompress savestate", nullptr, MB_ICONERROR);
		savestates_job_success = FALSE;
		return;
	}

	// BUG (PRONE): we arent allowed to hold on to a vector element pointer
	// find another way of doing this
	auto ptr = decompressed_buf.data();

    // compare current rom hash with one stored in state
	char md5[33] = {0};
    memread(&ptr, &md5, 32);

	if (memcmp(md5, rom_md5, 32)) {

		auto result = MessageBox(mainHWND, std::format("The savestate was created on a rom with hash {}, but is being loaded on another rom.\r\nThe emulator may crash. Are you sure you want to continue?", md5).c_str(), nullptr, MB_ICONQUESTION | MB_YESNO);

		if (result != IDYES) {
			savestates_job_success = FALSE;
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
        fprintf(stderr, "Snapshot event queue terminator not reached.\n");
        savestates_job_success = FALSE;
        Statusbar::post("Event queue too long (corrupted?)");
        savestates_job_success = FALSE;
        return;
    }

    uint32_t is_movie;
    memread(&ptr, &is_movie, sizeof(is_movie));

    if (is_movie)
    {
	    // this .st is part of a movie, we need to overwrite our current movie buffer

	    // hash matches, load and verify rest of the data
	    unsigned long movie_input_data_size = 0;
	    memread(&ptr, &movie_input_data_size, sizeof(movie_input_data_size));

	    const auto local_movie_data = (char*)malloc(movie_input_data_size);
	    memread(&ptr, local_movie_data, movie_input_data_size);

	    const int code = vcr_movie_unfreeze(local_movie_data, movie_input_data_size);
	    free(local_movie_data);

	    if (code != SUCCESS && !vcr_is_idle())
	    {
		    std::string err_str = "Failed to restore movie, ";
		    switch (code)
		    {
		    case NOT_FROM_THIS_MOVIE:
			    err_str += "snapshot not from this movie";
			    break;
		    case NOT_FROM_A_MOVIE:
			    err_str += "snapshot not from a movie";
			    break;
		    case INVALID_FRAME:
			    err_str += "invalid frame number";
			    break;
		    case WRONG_FORMAT:
			    err_str += "wrong format";
			    stop = true;
			    break;
		    default:
			    break;
		    }
		    auto result = MessageBox(mainHWND,
		                             (err_str +
			                             "\r\nAre you sure you want to continue?")
		                             .c_str(), nullptr,
		                             MB_ICONQUESTION | MB_YESNO);
		    if (result != IDYES)
		    {
			    if (vcr_is_recording())
			    {
				    vcr_stop_record();
			    }
			    if (vcr_is_playing())
			    {
				    vcr_stop_playback();
			    }
			    savestates_job_success = FALSE;
			    goto failedLoad;
		    }
	    }
    }
    else
    {
	    if (vcr_is_active())
	    {
	    	auto result = MessageBox(mainHWND, "Loading a non-movie savestate during movie playback might desynchronize playback.\r\nAre you sure you want to continue?", nullptr, MB_ICONQUESTION | MB_YESNO);
		    if (result != IDYES)
		    {
		    	savestates_job_success = FALSE;
		    	return;
		    }
	    }

	    // at this point we know the savestate is safe to be loaded (done after else block)
    }

    // so far loading success! overwrite memory
    load_eventqueue_infos(buf);
    load_memory_from_buffer(first_block);
    LuaCallbacks::call_load_state();
	if (st_medium == e_st_medium::path)
	{
		Statusbar::post(std::format("Loaded {}", new_st_path.filename().string()));
	}
	if (st_medium == e_st_medium::slot)
	{
		Statusbar::post(std::format("Loaded slot {}", st_slot + 1));
	}
failedLoad:
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

	printf("Savestate loading took %dms\n", static_cast<int>((std::chrono::high_resolution_clock::now() - start_time).count() / 1'000'000));
}

void savestates_do_file(const std::filesystem::path& path, const e_st_job job)
{
	st_path = path;
	savestates_job = job;
	st_medium = e_st_medium::path;
}

void savestates_do_memory(const std::string key, e_st_job job)
{
	st_key = key;
	savestates_job = job;
	st_medium = e_st_medium::memory;
}

void savestates_do_slot(const int32_t slot, const e_st_job job)
{
	savestates_set_slot(slot == -1 ? st_slot : slot);
	savestates_job = job;
	st_medium = e_st_medium::slot;
}

