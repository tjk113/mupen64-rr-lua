#ifndef COP1_HELPERS_H
#define COP1_HELPERS_H

#define CHECK_INPUT(x) \
    do { if (emulate_float_crashes && !isnormal(x) && (x) != 0 && !isinf(x)) { \
        printf("Operation on denormal/nan: %lf; PC = 0x%lx\n", x, PC->addr); \
        stop=1; \
    } } while (0)

#define CHECK_OUTPUT(x) \
    do { if (emulate_float_crashes && !isnormal(x) && !isinf(x)) { \
        if (isnan(x)) { \
            printf("Invalid float operation; PC = 0x%lx\n", PC->addr); \
            stop=1; \
        } \
        /* Flush denormals to zero manually, since x87 doesn't have a built-in */ \
        /* way to do it. Typically this doesn't matter, because denormals are */ \
        /* too small to cause visible console/emu divergences, but since we */ \
        /* check for them on entry to each operation this becomes important.. */ \
        x = 0; \
    } } while (0)

#endif /* COP1_HELPERS_H */
