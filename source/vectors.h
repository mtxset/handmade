/* date = September 23rd 2021 6:41 pm */

#ifndef MATH_H
#define MATH_H

#include "intrinsics.h"

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
    f32 e[3];
};

union v4 {
    struct {
        f32 x, y, z, w;
    };
    
    struct {
        f32 r, g, b, a;
    };
    f32 e[4];
};

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
bool is_zero_v2(v2 a)
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
f32 length_squared_v2(v2 a) {
    f32 result;
    
    result = inner(a, a);
    
    return result;
}

inline
f32 length_v2(v2 a) {
    f32 result;
    
    f32 dot = inner(a, a);
    result = square_root(dot);
    
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

#endif //MATH_H
