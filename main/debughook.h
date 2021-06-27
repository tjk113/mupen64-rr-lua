//MSVC ONLY, included with /FI option in debug target settings
#pragma once
#include <stdio.h>

size_t fwrite2(const void* ptr, size_t size, size_t count, FILE* stream);

#define fwrite(a,b,c,d) fwrite2(a,b,c,d)