/* date = September 13th 2021 11:30 am */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <math.h>
#include "platform.h"

union v2 {
    struct {
        f32 x, y;
    };
    f32 e[2];
};

union v3 {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
    struct {
        v2 xy;
        f32 _discard;
    };
    struct {
        f32 _discard;
        v2 yz;
    };
    f32 e[3];
};

union v4 {
    struct {
        f32 x, y, z, w;
    };
    
    struct {
        f32 r, g, b, a;
    };
    
    struct {
        v3 rgb;
        f32 _discard;
    };
    
    struct {
        v3 xyz;
        f32 _discard;
    };
    
    f32 e[4];
};

inline
f32
safe_ratio_n(f32 numerator, f32 divisor, f32 n) {
    f32 result = n;
    
    if (divisor != 0.0f) {
        result = numerator / divisor;
    }
    
    return result;
}

inline
f32
safe_ratio_0(f32 num, f32 div) {
    f32 result = safe_ratio_n(num, div, 0.0f);
    
    return result;
}

inline
f32
safe_ratio_1(f32 num, f32 div) {
    f32 result = safe_ratio_n(num, div, 1.0f);
    
    return result;
}

inline
f32 
clamp(f32 min_value, f32 value, f32 max_value) {
    f32 result = value;
    
    if (result < min_value)
        result = min_value;
    else if (result > max_value)
        result = max_value;
    
    return result;
}

inline
f32
clamp01(f32 value) {
    f32 result = clamp(0.0f, value, 1.0f);
    return result;
}

// VECTORS START

inline
v2
v2_i32(i32 x, i32 y) {
    v2 result;
    
    result = {(f32)x, (f32)y};
    
    return result;
}

inline
v2
v2_u32(u32 x, u32 y) {
    v2 result;
    
    result = {(f32)x, (f32)y};
    
    return result;
}

inline
v2 operator + (v2 a, v2 b) {
    v2 result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return result;
}

inline 
v2 operator - (v2 a, v2 b) {
    v2 result;
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return result;
}

inline
v2 operator += (v2 &a, v2 b) {
    a = a + b;
    return a;
}

inline
v2 operator -= (v2 &a, v2 b) {
    a = a - b;
    return a;
}

inline
v2 operator - (v2 a) {
    v2 result;
    
    result.x = -a.x;
    result.y = -a.y;
    
    return result;
}

inline
v2 operator * (f32 a, v2 b) {
    v2 result;
    
    result.x = b.x * a;
    result.y = b.y * a;
    
    return result;
}

inline
v2 operator * (v2 a, f32 b) {
    v2 result;
    
    result = b * a;
    
    return result;
}

inline
v2 operator *= (v2 &a, f32 b) {
    a = b * a;
    return a;
}

inline 
bool is_zero(v2 a)
{
    bool result = a.x == 0 && a.y == 0;
    return result;
}

inline
f32 inner(v2 a, v2 b) {
    f32 result;
    
    result = a.x * b.x + a.y * b.y;
    
    return result;
}

inline
v2
hadamard(v2 a, v2 b) {
    v2 result;
    
    result = { a.x * b.x, a.y * b.y };
    
    return result;
}

inline
f32 length_squared(v2 a) {
    f32 result;
    
    result = inner(a, a);
    
    return result;
}

inline
f32 
length(v2 a) {
    f32 result;
    
    f32 dot = inner(a, a);
    result = sqrtf(dot);
    
    return result;
}

inline
v2
clamp01(v2 value) {
    v2 result; 
    
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    
    return result;
}

inline
v3
clamp01(v3 value) {
    v3 result; 
    
    result.x = clamp01(value.x);
    result.y = clamp01(value.y);
    result.z = clamp01(value.z);
    
    return result;
}

inline
v2 perpendicular_v2(v2 a) {
    v2 result;
    
    result = { -a.y, a.x };
    
    return result;
}

inline
f32 arctan(f32 value) {
    f32 result; 
    
    result = (f32)atan(value);
    
    return result;
}

inline
u32 quadrant_v2(v2 a) {
    u32 result = 0;
    
    if      (a.x >= 0 && a.y >= 0) {
        result = 1;
    }
    else if (a.x < 0 && a.y >= 0) {
        result = 2;
    }
    else if (a.x < 0 && a.y < 0) {
        result = 3;
    }
    else if (a.x >= 0 && a.y < 0) {
        result = 4;
    }
    
    macro_assert(result > 0 && result < 5);
    
    return result;
}

inline
v3
v2_to_v3(v2 a, f32 z) {
    v3 result;
    
    result = v3{a.x, a.y, z};
    
    return result;
}

inline
v3 operator + (v3 a, v3 b) {
    v3 result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    
    return result;
}

inline
v3 operator += (v3 &a, v3 b) {
    a = a + b;
    return a;
}

inline 
v3 operator - (v3 a, v3 b) {
    v3 result;
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    
    return result;
}

inline
v3 operator -= (v3 &a, v3 b) {
    a = a - b;
    return a;
}

inline
v3 operator - (v3 a) {
    v3 result;
    
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    
    return result;
}

inline
v3 operator * (f32 a, v3 b) {
    v3 result;
    
    result.x = b.x * a;
    result.y = b.y * a;
    result.z = b.z * a;
    
    return result;
}

inline
v3 operator * (v3 a, f32 b) {
    v3 result;
    
    result = b * a;
    
    return result;
}

inline
v3 operator *= (v3 &a, f32 b) {
    a = b * a;
    return a;
}

inline
v3 operator / (f32 a, v3 b) {
    v3 result;
    
    result = b * (1 / a);
    
    return result;
}

inline
v3 operator / (v3 a, f32 b) {
    v3 result;
    
    result = (1 / b) * a;
    
    return result;
}

inline 
bool is_zero(v3 a)
{
    bool result = a.x == 0 && a.y == 0 && a.z == 0;
    return result;
}

inline
f32 inner(v3 a, v3 b) {
    f32 result;
    
    result = a.x * b.x + a.y * b.y + a.z * b.z; 
    
    return result;
}

inline
v3
hadamard(v3 a, v3 b) {
    v3 result;
    
    result = { a.x * b.x, a.y * b.y, a.z * b.z};
    
    return result;
}

inline
f32 dot(v3 a, v3 b) {
    return inner(a, b);
}

v3 cross(v3 a, v3 b) {
    v3 result;
    
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    
    return result;
}

inline
f32 length_squared(v3 a) {
    f32 result;
    
    result = inner(a, a);
    
    return result;
}

inline
f32 
length(v3 a) {
    f32 result;
    
    f32 dot = inner(a, a);
    result = sqrtf(dot);
    
    return result;
}


inline
v3
normalize(v3 a) {
    v3 result;
    
    result = a * (1.0f / length(a));
    
    return result;
}


inline
v3 
unit_vector(v3 a) {
    v3 result;
    
    result = a / length(a);
    
    return result;
}

inline
v3
lerp(v3 a, f32 t, v3 b) {
    v3 result;
    
    result = (1.0f - t) * a + t * b;
    
    return result;
}

inline
v4 operator + (v4 a, v4 b) {
    v4 result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    
    return result;
}

inline
v4 operator += (v4 &a, v4 b) {
    a = a + b;
    return a;
}


inline
v4 operator * (f32 a, v4 b) {
    v4 result;
    
    result.x = b.x * a;
    result.y = b.y * a;
    result.z = b.z * a;
    result.w = b.w * a;
    
    return result;
}

inline
v4 
operator * (v4 a, f32 b) {
    v4 result;
    
    result = b * a;
    
    return result;
}

inline
v4 
operator *= (v4 &a, f32 b) {
    a = b * a;
    return a;
}

inline
v4
lerp(v4 a, f32 t, v4 b) {
    v4 result;
    
    result = (1.0f - t) * a + t * b;
    
    return result;
}

inline
v4
hadamard(v4 a, v4 b) {
    v4 result;
    
    result = { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
    
    return result;
}

// VECTORS END


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
f32 
square(f32 value) {
    f32 result;
    
    result = value * value;
    
    return result;
}

inline
f32 
lerp(f32 a, f32 t, f32 b) {
    f32 result;
    
    result = (1.0f - t) * a + t * b;
    
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

// Rect2 START

struct Rect2 {
    v2 min;
    v2 max;
};

inline
v2
perpendicular(v2 a) {
    v2 result;
    
    result = {-a.y, a.x};
    
    return result;
}

inline
v2 
get_min_corner(Rect2 rect) {
    v2 result;
    
    result = rect.min;
    
    return result;
}

inline
v2 get_max_corner(Rect2 rect) {
    v2 result;
    
    result = rect.max;
    
    return result;
}

inline
v2 get_center(Rect2 rect) {
    v2 result;
    
    result = 0.5f * (rect.min + rect.max);
    
    return result;
}

inline
Rect2 rect_min_max(v2 min, v2 max) {
    Rect2 result;
    
    result.min = min;
    result.max = max;
    
    return result;
}

inline
Rect2 rect_min_dim(v2 min, v2 dim) {
    Rect2 result;
    
    result.min = min;
    result.max = min + dim;
    
    return result;
}

inline
Rect2 
rect_center_half_dim(v2 center, v2 half_dim) {
    Rect2 result;
    
    result.min = center - half_dim;
    result.max = center + half_dim;
    
    return result;
}

inline
Rect2 
rect_center_dim(v2 center, v2 dim) {
    Rect2 result;
    
    result = rect_center_half_dim(center, 0.5f * dim);
    
    return result;
}

inline
bool 
is_in_rect(Rect2 rect, v2 point) {
    bool result;
    
    result = 
        point.x >= rect.min.x && 
        point.x <  rect.max.x &&
        point.y >= rect.min.y &&
        point.y <  rect.max.y;
    
    return result;
}

inline
Rect2
add_radius_to(Rect2 rect, v2 radius) {
    Rect2 result;
    
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    
    return result;
}

inline
v2
get_barycentric(Rect2 a, v2 pos) {
    v2 result;
    
    result.x = safe_ratio_0(pos.x - a.min.x, a.max.x - a.min.x);
    result.y = safe_ratio_0(pos.y - a.min.y, a.max.y - a.min.y);
    
    return result;
}


// Rect2 END

// Rect3 START

struct Rect3 {
    v3 min;
    v3 max;
};

inline
Rect2
to_rect_xy(Rect3 rect) {
    Rect2 result;
    
    result.min = rect.min.xy;
    result.max = rect.max.xy;
    
    return result;
}

inline
v3 
get_min_corner(Rect3 rect) {
    v3 result;
    
    result = rect.min;
    
    return result;
}

inline
v3 get_max_corner(Rect3 rect) {
    v3 result;
    
    result = rect.max;
    
    return result;
}

inline
v3 get_center(Rect3 rect) {
    v3 result;
    
    result = 0.5f * (rect.min + rect.max);
    
    return result;
}

inline
Rect3 rect_min_max(v3 min, v3 max) {
    Rect3 result;
    
    result.min = min;
    result.max = max;
    
    return result;
}

inline
Rect3 rect_min_dim(v3 min, v3 dim) {
    Rect3 result;
    
    result.min = min;
    result.max = min + dim;
    
    return result;
}

inline
Rect3 
rect_center_half_dim(v3 center, v3 half_dim) {
    Rect3 result;
    
    result.min = center - half_dim;
    result.max = center + half_dim;
    
    return result;
}

inline
Rect3 
rect_center_dim(v3 center, v3 dim) {
    Rect3 result;
    
    result = rect_center_half_dim(center, 0.5f * dim);
    
    return result;
}

inline
bool 
is_in_rect(Rect3 rect, v3 point) {
    bool result;
    
    result = 
        point.x >= rect.min.x && 
        point.y >= rect.min.y &&
        point.z >= rect.min.z && 
        
        point.x <  rect.max.x &&
        point.y <  rect.max.y &&
        point.z <  rect.max.z ;
    
    return result;
}

inline
Rect3
add_radius_to(Rect3 rect, v3 radius) {
    Rect3 result;
    
    result.min = rect.min - radius;
    result.max = rect.max + radius;
    
    return result;
}

inline
Rect3
offset(Rect3 rect, v3 offset) {
    Rect3 result;
    
    result.min = rect.min + offset;
    result.max = rect.max + offset;
    
    return result;
}

inline
bool
rects_intersects(Rect3 a, Rect3 b) {
    bool result = false;
    
    result = !(b.max.x <= a.min.x || b.min.x >= a.max.x ||
               b.max.y <= a.min.y || b.min.y >= a.max.y ||
               b.max.z <= a.min.z || b.min.z >= a.max.z);
    
    return result;
}

inline
v3
get_barycentric(Rect3 a, v3 pos) {
    v3 result;
    
    result.x = safe_ratio_0(pos.x - a.min.x, a.max.x - a.min.x);
    result.y = safe_ratio_0(pos.y - a.min.y, a.max.y - a.min.y);
    result.z = safe_ratio_0(pos.z - a.min.z, a.max.z - a.min.z);
    
    return result;
}

// RECT3 END


#endif //INTRINSICS_H
