//MSVC ONLY
#include "debughook.h"
#undef fwrite
size_t fwrite2(const void* ptr, size_t size, size_t count, FILE* stream)
{
    printf("Writing %d bytes: ", size * count);
    for (int i = 0; i < size * count; i++)
    {
        // crashes if you are writing to weird place btw
        printf("%hhx", ((const char*)ptr)[i]);
    }
    printf("\n");
    return fwrite(ptr, size, count, stream);
}
