// a libc replacement (more or less) for the Microsoft Visual C compiler
// this file is public domain -- do with it whatever you want!
#ifndef __LIBC_H_INCLUDED__
#define __LIBC_H_INCLUDED__

#ifdef _MSC_VER
    #define INLINE __forceinline
    #define FASTCALL __fastcall
    #ifdef NOLIBC
        #ifdef MAIN_PROGRAM
            int _fltused=0;
        #endif
    #endif
#else
    #define INLINE inline
    #define FASTCALL __attribute__((fastcall))
    #include <stdint.h>
#endif

#ifdef _WIN32
    #ifndef WIN32
        #define WIN32
    #endif
#endif
#ifdef WIN32
    #include <windows.h>
#endif

#if !NEED_MINILIBC
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#endif
#include <math.h>

#ifndef __int8_t_defined
    #define __int8_t_defined
    typedef unsigned char  uint8_t;
    typedef   signed char   int8_t;
    typedef unsigned short uint16_t;
    typedef   signed short  int16_t;
    typedef unsigned int   uint32_t;
    typedef   signed int    int32_t;
    #ifdef _MSC_VER
        typedef unsigned __int64 uint64_t;
        typedef   signed __int64  int64_t;
    #elif defined(__x86_64__) && defined(__linux__)
        #include <sys/types.h>
    #else
        typedef unsigned long long uint64_t;
        typedef   signed long long  int64_t;
    #endif
#endif

#ifndef NULL
    #define NULL 0
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979
#endif

#define libc_malloc  malloc
#define libc_calloc  calloc
#define libc_realloc realloc
#define libc_free    free

#define libc_memset  memset
#define libc_memcpy  memcpy
#define libc_memmove memmove

#if defined(_MSC_VER) && !defined(_DEBUG) 
static INLINE double libc_frexp(double x, int *e) {
    double res = -9999.999;
    unsigned __int64 i = *(unsigned __int64*)(&x);
    if (!(i & 0x7F00000000000000UL)) {
        *e = 0;
        return x;
    }
    *e = ((i << 1) >> 53) - 1022;
    i &= 0x800FFFFFFFFFFFFFUL;
    i |= 0x3FF0000000000000UL;
    return *(double*)(&i) * 0.5;
}
#else
    #define libc_frexp  frexp
#endif

#define libc_exp     exp
#define libc_pow     pow

#endif//__LIBC_H_INCLUDED__
