#pragma once

#define BIT_GET(val,n)   ((val>>n)&1)
#define BIT_SET(val,n,f) ((val&~(1<<n))|(f<<n))
#define BYTE_GET(val,n)  ((val >> (8*n)) & 0xff)

