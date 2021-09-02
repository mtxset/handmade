#include <stdint.h>
#include "types.h"

template <typename T> void 
swap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

u32 
truncate_u64(u64 value) {
    macro_assert(value <= UINT32_MAX);
    return (u32)value;
}