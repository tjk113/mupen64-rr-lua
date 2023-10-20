//MSVC ONLY, included with /FI option in debug target settings
#pragma once
#include <stdio.h>

size_t fwrite2(const void* ptr, size_t size, size_t count, FILE* stream);

// Only use debug (much slower) fwrite on debug target
#ifdef _DEBUG
#define fwrite(a,b,c,d) fwrite2(a,b,c,d)
#endif
