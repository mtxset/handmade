/* date = August 6th 2021 1:58 am */

#ifndef TYPES_H
#define TYPES_H

#include <intrin.h>

typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

typedef intptr_t           intptr;
typedef uintptr_t          uintptr;

#define _(x)

#if !defined(internal)
#define internal static
#endif

#define global_var static
#define local_persist static

#endif //TYPES_H
