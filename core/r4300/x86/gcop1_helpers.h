#ifndef GCOP1_HELPERS_H
#define GCOP1_HELPERS_H

void gencheck_float_input_valid(int32_t stackBase);
void gencheck_float_output_valid();
void gencheck_float_conversion_valid();

extern float largest_denormal_float;
extern double largest_denormal_double;

#endif // GCOP1_HELPERS_H
