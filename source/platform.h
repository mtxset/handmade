/* date = September 20th 2021 7:12 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM

#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#endif //!COMPILER_MSVC && !COMPILER_LLVM

#if COMPILER_MSVC
#include <intrin.h>
inline u32 atomic_compare_exchange_u32(u32 volatile *value, u32 expected, u32 new_value) {
  u32 result = InterlockedCompareExchange(value, expected, new_value);
  return result;
}
#endif

#include <float.h>

global_var f32 MAX_F32 = 3.402823466e+38F;
global_var f32 MIN_F32 = 1.175494351e-38F;

#define align_16(value) ((value + 15) & ~15)

#endif //PLATFORM_H
