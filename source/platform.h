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
#endif



#endif //PLATFORM_H
