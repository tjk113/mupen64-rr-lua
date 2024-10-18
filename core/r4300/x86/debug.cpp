/**
 * Mupen64 - debug.c
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


#include "assemble.h"
#include <stdio.h>
#include "../r4300.h"
#include "../macros.h"
#include "../../memory/memory.h"
#include "../recomph.h"

void debug()
{
#ifdef COMPARE_CORE
	compare_core();
#endif
    //if (Count > 0x8000000)
    //g_core_logger->info("PC->addr:{:#06x}:{:#06x}\n",(int)PC->addr,
    /*(int)SP_DMEM[(PC->addr - 0xa4000000)/4]*/
    //(int)rdram[(PC->addr & 0xFFFFFF)/4]);
    //g_core_logger->info("count:{:#06x}\n", (int)(Count));
    /*if (debug_count + Count >= 0x80000000)
      g_core_logger->info("debug : {:#06x}: {:#06x}\n",
         (unsigned int)(PC->addr),
         (unsigned int)rdram[(PC->addr&0xFFFFFF)/4]);*/
    /*if (debug_count + Count >= 0x8000000) {
       g_core_logger->info("debug : {:#06x}\n", (unsigned int)(PC->addr));
       if (0x8018ddd8>actual->debut && 0x8018ddd8<actual->fin) {
      g_core_logger->info("ff: {:#06x}\n", //rdram[0x18ddd8/4]
            actual->code[actual->block[(0x8018ddd8-actual->debut)/4].local_addr]);
      getchar();
       }
    }*/
    //if (debug_count + Count >= 0x8000000) actual->code[(PC+1)->local_addr] = 0xC3;
    //if ((debug_count + Count) >= 0x5f66c82)
    //if ((debug_count + Count) >= 0x5f61bc0)
    /*if ((debug_count + Count) == 0xf203ae0)
      {
     int j;
     for (j=0; j<NBR_BLOCKS; j++)
       {
          if (aux[j].debut) {
         g_core_logger->info("deb:{:#06x}", aux[j].debut);
         g_core_logger->info("fin:{:#06x}", aux[j].fin);
         g_core_logger->info("valide:{:#06x}", aux[j].valide);
         getchar();
          }
       }
      }
    if ((debug_count + Count) >= 0xf203ae0)
      {
     int j;
     g_core_logger->info ("inst:{:#06x}\n",
         (unsigned int)rdram[(PC->addr&0xFFFFFF)/4]);
     g_core_logger->info ("PC={:#06x}\n", (unsigned int)((PC+1)->addr));
     for (j=0; j<16; j++)
       g_core_logger->info ("reg[{}]:{:#06x}{:#06x}       reg[{}]:{:#06x}{:#06x}\n",
           j,
           (unsigned int)(reg[j] >> 32),
           (unsigned int)reg[j],
           j+16,
           (unsigned int)(reg[j+16] >> 32),
           (unsigned int)reg[j+16]);
     g_core_logger->info("hi:{:#06x}{:#06x}        lo:{:#06x}{:#06x}\n",
            (unsigned int)(hi >> 32),
            (unsigned int)hi,
            (unsigned int)(lo >> 32),
            (unsigned int)lo);
     g_core_logger->info("après {} instructions soit {:#06x}\n",
            (unsigned int)(debug_count+Count),
            (unsigned int)(debug_count+Count));
     getchar();
      }*/
}

//static void dyna_stop() {}
