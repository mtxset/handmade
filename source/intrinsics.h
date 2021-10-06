/* date = September 13th 2021 11:30 am */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "math.h"
#include "platform.h"

struct Bit_scan_result {
    bool found;
    u32 index;
};

inline
i32 sign_of(i32 value) {
    i32 result;
    
    result = value >= 0 ? 1 : -1;
    
    return result;
}

i32 round_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)roundf(value);
    // 0.5 cuz c will truncate decimal points, so 0.6 will be 0
    
    return result;
}

u32 round_f32_u32(f32 value) {
    u32 result;
    
    result = (u32)roundf(value);
    
    return result;
}

u32 truncate_u64_u32(u64 value) {
    macro_assert(value <= UINT32_MAX);
    return (u32)value;
}

i32 truncate_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)value;
    
    return result;
}

inline
i32 ceil_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)ceilf(value);
    
    return result;
}

i32 floor_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)floorf(value);
    
    return result;
}

f32 sin(f32 value) {
    f32 result = 0;
    
    result = sinf(value);
    
    return result;
}

f32 cos(f32 value) {
    f32 result = 0;
    
    result = cosf(value);
    
    return result;
}

f32 atan2(f32 x, f32 y) {
    f32 result = 0;
    
    result = atan2f(x, y);
    
    return result;
}

inline
Bit_scan_result find_least_significant_first_bit(u32 value) {
    Bit_scan_result result = {};
    
#if 0
    result.found = (_BitScanForward((unsigned long*)&result.index, value) > 0);
#else
    for (u32 i = 0; i < 32; i++) {
        if (value & (1 << i)) {
            result.index = i;
            result.found = true;
            break;
        }
    }
#endif
    
    return result;
}

inline
f32 square(f32 value) {
    f32 result;
    
    result = value * value;
    
    return result;
}

inline
f32 square_root(f32 value) {
    f32 result;
    
    result = sqrtf(value);
    
    return result;
}

inline
f32 absolute(f32 value) {
    f32 result;
    
    // fabsf casts double to float
    result = (f32)fabs(value);
    
    return result;
}

inline
u32 rotate_left(u32 value, i32 shift) {
    u32 result;
    
    result = _rotl(value, shift);
    
    return result;
}

inline
u32 rotate_right(u32 value, i32 shift) {
    u32 result;
    
    result = _rotr(value, shift);
    
    return result;
}

inline
f32 rad_to_degrees(f32 value) {
    f32 result;
    
    result = value * (180.0f / PI);
    
    return result;
}

#endif //INTRINSICS_H
