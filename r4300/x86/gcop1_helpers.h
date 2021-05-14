#ifndef GCOP1_HELPERS_H
#define GCOP1_HELPERS_H

void gencheck_float_input_valid();
void gencheck_float_output_valid();
void gencheck_float_conversion_valid();

extern const unsigned int largest_denormal_float;
extern const unsigned long long largest_denormal_double;

#endif // GCOP1_HELPERS_H
