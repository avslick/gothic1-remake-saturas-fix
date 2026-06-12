#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
  #include <windows.h>
  #include <intrin.h>
  #define __forceinline_compat
#else
  #include <x86intrin.h>
  #define _byteswap_ulong(x) __builtin_bswap32(x)
  #define _byteswap_uint64(x) __builtin_bswap64(x)
  #define _byteswap_ushort(x) __builtin_bswap16(x)
  #define __forceinline inline __attribute__((always_inline))
  #define _BitScanReverse(idx, mask) (*(idx) = 31 - __builtin_clz(mask), (mask)!=0)
  #define _BitScanForward(idx, mask) (*(idx) = __builtin_ctz(mask), (mask)!=0)
#endif

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned int uint32;
typedef uint64_t uint64;
typedef int64_t int64;
typedef signed int int32;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint;
