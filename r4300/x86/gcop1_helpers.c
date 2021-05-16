#include <stdio.h>
#include "../recomph.h"
#include "../r4300.h"
#include "../exception.h"
#include "../macros.h"
#include "assemble.h"
#include "gcop1_helpers.h"

const unsigned int largest_denormal_float = (1U << 23) - 1;
const unsigned long long largest_denormal_double = (1ULL << 52) - 1;

static void check_failed()
{
    printf("Operation on denormal/nan; PC = 0x%lx\n", PC->addr);
    Cause = 15 << 2;
    exception_general();
}

static void post_check_failed()
{
    printf("Operation resulted in nan; PC = 0x%lx\n", PC->addr);
    Cause = 15 << 2;
    exception_general();
}

static void conversion_failed()
{
    printf("Out-of-range float conversion; PC = 0x%lx\n", PC->addr);
    Cause = 15 << 2;
    exception_general();
}

static void patch_jump(unsigned long addr, unsigned long target) {
    (*inst_pointer)[addr - 1] = (unsigned char)(target - addr);
}

/**
 * Given the fp stack:
 * ST(0) = x
 * ST(1) = largest denormal (float/double)
 * Assert that x is not denormal or nan, ending with a cleared stack.
 */
void gencheck_float_input_valid()
{
    // if abs(x) > largest denormal, goto A
    fabs_(); // ST(0) = abs(ST(0))
    fucomi_fpreg(1); // compare ST(0) <=> ST(1)
    fstp_fpreg(1); // pop ST(1)
    ja_rj(0);
    unsigned long jump1 = code_length;

    // if abs(x) == 0, goto A
    fldz(); // push zero
    fucomip_fpreg(1); // compare ST(0) <=> ST(1), pop
    je_rj(0);
    unsigned long jump2 = code_length;

    fstp_fpreg(0); // pop
    gencallinterp((unsigned long)check_failed, 0);
    fldz(); // push zero to balance stack (immediately popped below)

    // A:
    patch_jump(jump1, code_length);
    patch_jump(jump2, code_length);
    fstp_fpreg(0); // pop
}

/**
 * Given the fp stack:
 * ST(0) = largest denormal (float/double)
 * ST(1) = x
 * Assert that x is not nan, and flush denormal x to zero, leaving a
 * replacement value on the fp stack.
 */
void gencheck_float_output_valid()
{
    // if abs(x) > largest denormal, goto B
    fld_fpreg(1); // duplicate ST(1)
    fabs_(); // ST(0) = abs(ST(0))
    fucomip_fpreg(1); // compare ST(0) <=> ST(1), pop
    fstp_fpreg(0); // pop
    ja_rj(0);
    unsigned long jump1 = code_length;

    fucomip_fpreg(0); // compare ST(0) <=> ST(0), pop
    je_rj(0); // if equal (i.e. x was not nan), goto A
    unsigned long jump2 = code_length;

    gencallinterp((unsigned long)post_check_failed, 0);

    // A:
    patch_jump(jump2, code_length);
    // replace the (denormal or zero) result by zero (see CHECK_OUTPUT in
    // cop1_helpers.h for reasoning)
    // TODO: negative zero :(
    fldz();

    // B:
    patch_jump(jump1, code_length);
}

/**
 * Assert that the last x87 operation did not result in a (masked) Invalid
 * Operation exception. fclex must be called immediately before the operation.
 *
 * Clobbers eax.
 */
void gencheck_float_conversion_valid()
{
    fstsw_ax();
    test_al_imm8(1); // Invalid Operation bit
    je_rj(0); // jump if not set (i.e. ZF = 1)
    unsigned long jump1 = code_length;

    gencallinterp((unsigned long)conversion_failed, 0);

    patch_jump(jump1, code_length);
}
