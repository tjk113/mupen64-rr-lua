/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <cstdlib>

#include "../recomp.h"
#include "../r4300.h"
#include "../macros.h"
#include "../ops.h"
#include "../recomph.h"
#include <csetjmp>
#include <shared/services/LoggingService.h>
#include <shared/Messenger.h>

// NOTE: dynarec isn't compatible with the game debugger

void dyna_jump()
{
    if (PC->reg_cache_infos.need_map)
        *return_address = (uint32_t)(PC->reg_cache_infos.jump_wrapper);
    else
        *return_address = (uint32_t)(actual->code + PC->local_addr);
}

jmp_buf g_jmp_state;

void dyna_start(void (*code)())
{
    // code() ‚Ì‚Ç‚±‚©‚Å stop ‚ª true ‚É‚È‚Á‚½ŽžAdyna_stop() ‚ªŒÄ‚Î‚êAlongjmp() ‚Å setjmp() ‚µ‚½‚Æ‚±‚ë‚É–ß‚é
    // –ß‚Á‚Ä‚«‚½ setjmp() ‚Í 1 ‚ð•Ô‚·‚Ì‚ÅAdyna_start() I—¹
    // ƒŒƒWƒXƒ^ ebx, esi, edi, ebp ‚Ì•Û‘¶‚Æ•œŒ³‚ª•K—v‚¾‚ªAsetjmp() ‚ª‚â‚Á‚Ä‚­‚ê‚é
    core_executing = true;
	Messenger::broadcast(Messenger::Message::CoreExecutingChanged, core_executing);
    g_core_logger->info(L"core_executing: {}", (bool)core_executing);
    if (setjmp(g_jmp_state) == 0)
    {
        code();
    }
}

void dyna_stop()
{
    longjmp(g_jmp_state, 1); // goto dyna_start()
}
