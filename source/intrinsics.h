/* date = September 13th 2021 11:30 am */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "math.h"

inline
i32 round_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)(value + 0.5f);
    // 0.5 cuz c will truncate decimal poi32s, so 0.6 will be 0
    
    return result;
}

inline 
u32 round_f32_u32(f32 value) {
    u32 result;
    
    result = (u32)(value + 0.5f);
    
    return result;
}

inline 
u32 truncate_u64_u32(u64 value) {
    macro_assert(value <= UINT32_MAX);
    return (u32)value;
}

inline
i32 truncate_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)value;
    
    return result;
}


inline
i32 floor_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)floorf(value);
    
    return result;
}

inline
f32 sin(f32 value) {
    f32 result = 0;
    
    result = sinf(value);
    
    return result;
}

inline
f32 cos(f32 value) {
    f32 result = 0;
    
    result = cosf(value);
    
    return result;
}

inline
f32 atan2(f32 x, f32 y) {
    f32 result = 0;
    
    result = atan2f(x, y);
    
    return result;
}

#endif //INTRINSICS_H
