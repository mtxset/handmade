/* date = May 9th 2024 1:58 pm */
#ifndef MATH_H
#define MATH_H

#include <limits.h>
#include <math.h>

union v2 {
  struct { f32 x, y; };
  struct { f32 u, v; };
  struct { f32 width, height; };
  f32 e[2];
};

const v2 v2_zero = {0, 0}, v2_one = {1, 1}, v2_x = {1, 0}, v2_y = {0, 1}, v2_half = {.5, .5};
inline v2 V2(f32 x, f32 y) { return v2{x, y}; }

union v2i32 {
  struct {
    i32 x, y;
  };
  i32 e[2];
};

union v3 {
  struct {
    f32 x, y, z;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    f32 u, v, w;
  };
  
  struct {
    v2 xy;
    f32 _discard_z;
  };
  struct {
    f32 _discard_xy;
    v2 yz;
  };
  
  struct {
    v2 uv;
    f32 _discard_w;
  };
  struct {
    f32 _discard_uv;
    v2 vw;
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
  
  struct {
    v2 xy;
    f32 _discard0;
    f32 _discard1;
  };
  
  struct {
    f32 _discard0;
    v2 yz;
    f32 _discard1;
  };
  
  struct {
    f32 _discard0;
    f32 _discard1;
    v2 zw;
  };
  
  f32 e[4];
};

const v3 v3_zero = {0, 0, 0}, v3_one = {1, 1, 1}, v3_x = {1, 0, 0}, v3_y = {0, 1, 0}, v3_half = {.5, .5, .5};
inline v3 V3(v2 vec2, f32 z) {return v3{vec2.x, vec2.y, z}; };
inline v3 V3(f32 x, f32 y, f32 z) {return v3{x, y, z}; };

inline v4 V4(v3 vec3, f32 w) {return v4{vec3.x, vec3.y, vec3.z, w}; }
inline v4 V4(f32 a, f32 b, f32 c, f32 d) {return v4{a, b, c, d}; }

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

inline
i32 
clamp_i32(i32 val, i32 min_value = 0, i32 max_value = 1) {
  i32 result = val;
  
  if (val < min_value)
    result = min_value;
  
  if (val > max_value)
    result = max_value;
  
  return result;
}

inline
u32 
clamp_u32(u32 val, u32 min_value = 0, u32 max_value = 1) {
  u32 result = val;
  
  if (val < min_value)
    result = min_value;
  
  if (val > max_value)
    result = max_value;
  
  return result;
}

inline
f32 
clamp_f32(f32 val, f32 min_value = 0.0f, f32 max_value = 1.0f) {
  f32 result = val;
  
  if (val < min_value)
    result = min_value;
  
  if (val > max_value)
    result = max_value;
  
  return result;
}

template <typename T> 
void swap(T& a, T& b) {
  T c(a);
  a = b;
  b = c;
}

inline
f32
clamp_map_to_range(f32 min, f32 x, f32 max) {
  f32 result = 0;
  f32 range = max - min;
  
  if (range != 0) {
    result = clamp01((x - min) / range);
  }
  
  return result;
}

// VECTORS START

inline
v2
v2_i32(v2i32 a) {
  v2 result;
  
  result = {(f32)a.x, (f32)a.y};
  
  return result;
}

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
v2 operator += (v2 &a, v2 b) {
  a = a + b;
  return a;
}

inline 
v2 operator - (v2 a, v2 b) {
  v2 result;
  
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  
  return result;
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
v2
arm(f32 angle) {
  v2 result = v2 {cosf(angle), sinf(angle)};
  
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
  
  assert(result > 0 && result < 5);
  
  return result;
}

inline
v2
v3_to_v2(v3 a) {
  v2 result;
  
  result = {a.x, a.y};
  
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

inline
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

inline
v4
to_v4(v3 xyz, f32 w) {
  v4 result;
  
  result.xyz = xyz;
  result.w = w;
  
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
f32 sign_of(f32 value) {
  f32 result;
  
  result = value >= 0 ? 1.0f : -1.0f;
  
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
f32 atan2(f32 y, f32 x) {
  f32 result = 0;
  
  result = atan2f(y, x);
  
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

inline
v2
lerp(v2 a, f32 t, v2 b) {
  v2 result;
  
  result = (1.0f - t) * a + t * b;
  
  return result;
}

inline
v2
lerp_p3(v2 a, v2 b, v2 c, f32 t) {
  v2 result;
  
  v2 lerp_ab = lerp(a, t, b);
  v2 lerp_bc = lerp(b, t, c);
  
  result = lerp(lerp_ab, t, lerp_bc);
  
  return result;
}

inline
v2
lerp_p4(v2 a, v2 b, v2 c, v2 d, f32 t) {
  v2 result;
  
  v2 lerp_abc = lerp_p3(a, b, c, t);
  v2 lerp_bcd = lerp_p3(b, c, d, t);
  
  result = lerp(lerp_abc, t, lerp_bcd);
  
  return result;
}

// Rect2 START

introspect(category:"rect2") struct Rect2 {
  v2 min;
  v2 max;
};

struct Rect2i {
  i32 min_x, min_y, max_x, max_y;
};

inline
Rect2i
rect_intersect(Rect2i a, Rect2i b) {
  Rect2i result;
  
  result.min_x = (a.min_x < b.min_x) ? b.min_x : a.min_x;
  result.min_y = (a.min_y < b.min_y) ? b.min_y : a.min_y;
  result.max_x = (a.max_x > b.max_x) ? b.max_x : a.max_x;
  result.max_y = (a.max_y > b.max_y) ? b.max_y : a.max_y;
  
  return result;
}

inline
Rect2
rect2_union(Rect2 a, Rect2 b) {
  
  Rect2 result;
  
  result.min.x = (a.min.x < b.min.x) ? a.min.x : b.min.x;
  result.min.y = (a.min.y < b.min.y) ? a.min.y : b.min.y;
  
  result.max.x = (a.max.x > b.max.x) ? a.max.x : b.max.x;
  result.max.y = (a.max.y > b.max.y) ? a.max.y : b.max.y;
  
  return result;
}

inline
Rect2i
rect2i_union(Rect2i a, Rect2i b) {
  Rect2i result;
  
  result.min_x = (a.min_x < b.min_x) ? a.min_x : b.min_x;
  result.min_y = (a.min_y < b.min_y) ? a.min_y : b.min_y;
  result.max_x = (a.max_x > b.max_x) ? a.max_x : b.max_x;
  result.max_y = (a.max_y > b.max_y) ? a.max_y : b.max_y;
  
  return result;
}

inline
i32 
rect_clamp_area(Rect2i a) {
  i32 result = 0;
  
  i32 width  = a.max_x - a.min_x;
  i32 height = a.max_y - a.min_y;
  
  if (width > 0 && height > 0)
    result = width * height;
  
  return result;
}

inline
bool
rect_has_area(Rect2i a) {
  bool result;
  
  result = (a.min_x < a.max_x) && (a.min_y < a.max_y);
  
  return result;
}


inline
Rect2
rect2_inverted_infinity() {
  Rect2 result;
  
  result.min.x = result.min.y = FLT_MAX;
  result.max.x = result.max.y = -FLT_MAX;
  
  return result;
}


inline
Rect2i
rect2i_inverted_infinity() {
  Rect2i result;
  
  result.min_x = result.min_y = INT_MAX;
  result.max_x = result.max_y = -INT_MAX;
  
  return result;
}

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
v2 get_dim(Rect2 rect) {
  v2 result;
  
  result = rect.max - rect.min;
  
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

introspect(category:"rect2") struct Rect3 {
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
v3 get_dim(Rect3 rect) {
  v3 result;
  
  result = rect.max - rect.min;
  
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
Rect2
offset(Rect2 rect, v2 offset) {
  Rect2 result;
  
  result.min = rect.min + offset;
  result.max = rect.max + offset;
  
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
  
  result = !(b.max.x <= a.min.x || 
             b.min.x >= a.max.x ||
             b.max.y <= a.min.y || 
             b.min.y >= a.max.y ||
             b.max.z <= a.min.z || 
             b.min.z >= a.max.z);
  
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


inline
v4
srgb255_to_linear1(v4 color) {
  v4 result;
  
  f32 inv_255 = 1.0f / 255.0f;
  
  result.r = square(inv_255 * color.r);
  result.g = square(inv_255 * color.g);
  result.b = square(inv_255 * color.b);
  result.a = inv_255 * color.a;
  
  return result;
}

inline
v4
linear1_to_srgb255(v4 color) {
  v4 result;
  
  f32 one_255 = 255.0f;
  
  result.a = one_255 * color.a;
  result.r = one_255 * square_root(color.r);
  result.g = one_255 * square_root(color.g);
  result.b = one_255 * square_root(color.b);
  
  return result;
}

#endif //MATH_H
