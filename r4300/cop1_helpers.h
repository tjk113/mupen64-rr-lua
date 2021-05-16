#ifndef COP1_HELPERS_H
#define COP1_HELPERS_H

#include <stdio.h>
#include <math.h>
#include "r4300.h"
#include "exception.h"
#include "macros.h"

#define CHECK_INPUT(x) \
    do { if (emulate_float_crashes && !isnormal(x) && (x) != 0 && !isinf(x)) { \
        printf("Operation on denormal/nan: %lf; PC = 0x%lx\n", x, interpcore ? interp_addr : PC->addr); \
        Cause = 15 << 2; \
        exception_general(); \
        return; \
    } } while (0)

#define CHECK_OUTPUT(x) \
    do { if (emulate_float_crashes && !isnormal(x) && !isinf(x)) { \
        if (isnan(x)) { \
            printf("Invalid float operation; PC = 0x%lx\n", interpcore ? interp_addr : PC->addr); \
            Cause = 15 << 2; \
            exception_general(); \
            return; \
        } else { \
            /* Flush denormals to zero manually, since x87 doesn't have a built-in */ \
            /* way to do it. Typically this doesn't matter, because denormals are */ \
            /* too small to cause visible console/emu divergences, but since we */ \
            /* check for them on entry to each operation this becomes important... */ \
            x = copysign(0, x); \
        } \
    } } while (0)

#define CHECK_CONVERT_EXCEPTIONS() \
    do { read_x87_status_word(); if (x87_status_word & 1) { \
        printf("Out-of-range float conversion; PC = 0x%lx\n", interpcore ? interp_addr : PC->addr); \
        Cause = 15 << 2; \
        exception_general(); \
        return; \
    } } while (0)


#endif /* COP1_HELPERS_H */
