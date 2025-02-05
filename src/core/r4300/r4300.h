/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#ifdef _DEBUG
#define VR_PROFILE (1)
#endif

#include <core/r4300/recomp.h>
#include <core/memory/tlb.h>
#include <core/r4300/rom.h>
#include <core/include/core_types.h>

extern std::recursive_mutex g_emu_cs;

extern precomp_instr* PC;
extern uint32_t vr_op;

extern precomp_block *blocks[0x100000], *actual;
extern void (*interp_ops[64])(void);
extern int32_t fast_memory;
extern bool g_vr_beq_ignore_jmp;
extern volatile bool emu_launched;
extern volatile bool emu_paused;
extern volatile bool core_executing;
extern size_t g_total_frames;
extern int32_t stop, llbit;
extern int64_t reg[32], hi, lo;
extern int64_t local_rs, local_rt;
extern uint32_t reg_cop0[32];
extern int32_t local_rs32, local_rt32;
extern uint32_t jump_target;
extern double* reg_cop1_double[32];
extern float* reg_cop1_simple[32];
extern int32_t reg_cop1_fgr_32[32];
extern int64_t reg_cop1_fgr_64[32];
extern int32_t FCR0, FCR31;
extern tlb tlb_e[32];
extern uint32_t delay_slot, skip_jump, dyna_interp;
extern uint64_t debug_count;
extern uint32_t dynacore;
extern uint32_t interpcore;
extern uint32_t next_interrupt, CIC_Chip;
extern int32_t rounding_mode, trunc_mode, round_mode, ceil_mode, floor_mode;
extern int16_t x87_status_word;
extern uint32_t last_addr, interp_addr;
extern char invalid_code[0x100000];
extern uint32_t jump_to_address;
extern std::atomic<bool> screen_invalidated;
extern int32_t vi_field;
extern uint32_t next_vi;
extern int32_t compare_core_mode;
extern bool g_vr_fast_forward;
extern bool g_vr_frame_skipped;

extern FILE* g_eeprom_file;
extern FILE* g_sram_file;
extern FILE* g_fram_file;
extern FILE* g_mpak_file;

extern bool g_vr_benchmark_enabled;
extern std::atomic<int32_t> g_vr_wait_before_input_poll;

void pure_interpreter();
void compare_core();
extern void jump_to_func();
void update_count();
int32_t check_cop1_unusable();
void terminate_emu();

core_result vr_reset_rom_impl(bool reset_save_data, bool stop_vcr, bool skip_reset_recording_check = false);

#define jump_to(a) { jump_to_address = a; jump_to_func(); }

// Mask all exceptions, and set precision to 53 bits
#define TRUNC_MODE 0xE3F
#define ROUND_MODE 0x23F
#define CEIL_MODE 0xA3F
#define FLOOR_MODE 0x63F