/* date = September 13th 2021 11:30 am */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <math.h>
#include "platform.h"
#include "vectors.h"

struct Rect {
    v2 min;
    v2 max;
};

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

inline
i32 round_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)roundf(value);
    
    return result;
}

inline
u32 round_f32_u32(f32 value) {
    u32 result;
    
    result = (u32)roundf(value);
    
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
i32 ceil_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)ceilf(value);
    
    return result;
}

inline
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

inline
Bit_scan_result find_least_significant_first_bit(u32 value) {
    Bit_scan_result result = {};
    
#if COMPILER_MSVC
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
#if COMPILER_MSVC
    result = _rotl(value, shift);
#else
    // no idea
    result &= 31;
    result = value << shift | value >> (32 - shift);
#endif
    
    return result;
}

inline
u32 rotate_right(u32 value, i32 shift) {
    u32 result;
#if COMPILER_MSVC
    result = _rotr(value, shift);
#else
    // no idea
    result &= 31;
    result = value >> shift | value << (32 - shift);
#endif
    
    return result;
}

inline
f32 rad_to_degrees(f32 value) {
    f32 result;
    
    result = value * (180.0f / PI);
    
    return result;
}

inline
v2 get_min_corner(Rect rect) {
    v2 result;
    
    result = rect.min;
    
    return result;
}

inline
v2 get_max_corner(Rect rect) {
    v2 result;
    
    result = rect.max;
    
    return result;
}

inline
v2 get_center(Rect rect) {
    v2 result;
    
    result = 0.5f * (rect.min + rect.max);
    
    return result;
}

inline
Rect rect_min_max(v2 min, v2 max) {
    Rect result;
    
    result.min = min;
    result.max = max;
    
    return result;
}

inline
Rect rect_min_dim(v2 min, v2 dim) {
    Rect result;
    
    result.min = min;
    result.max = min + dim;
    
    return result;
}

inline
Rect rect_center_half_dim(v2 center, v2 half_dim) {
    Rect result;
    
    result.min = center - half_dim;
    result.max = center + half_dim;
    
    return result;
}

inline
Rect rect_center_dim(v2 center, v2 dim) {
    Rect result;
    
    result = rect_center_half_dim(center, 0.5f * dim);
    
    return result;
}

inline
bool is_in_rect(Rect rect, v2 point) {
    bool result;
    
    result = 
        point.x >= rect.min.x && 
        point.x <  rect.max.x &&
        point.y >= rect.min.y &&
        point.y <  rect.max.y;
    
    return result;
}

#endif //INTRINSICS_H
