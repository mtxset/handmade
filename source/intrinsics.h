/* date = September 13th 2021 11:30 am */

#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "math.h"
#if COMPILER_MSVC

#include <intrin.h>
inline u32 
atomic_compare_exchange_u32(u32 volatile *value, u32 new_value, u32 expected) {
  u32 result = _InterlockedCompareExchange((long*)value, new_value, expected);
  return result;
}


inline
u64
atomic_add_u64(u64 volatile *value, u64 addend) {
  
  u64 result = _InterlockedExchangeAdd64((__int64*)value, addend);
  
  return result;
}

inline
u64
atomic_exchange_u64(u64 volatile *value, u64 new_value) {
  u64 result = _InterlockedExchange64((__int64*)value, new_value);
  
  return result;
}
#endif

//
/*
inline
__m128i 
mm_or(__m128i a, __m128i b) {
    __m128i result;
    result = _mm_or_si128(a, b);
    return result;
}

inline
__m128i mm_convert_f_i(__m128 value) {
    __m128i result;
    result = _mm_cvttps_epi32(value);
    return result;
}

inline
__m128i mm_cast_f_i(__m128 value) {
    __m128i result;
    result = _mm_castps_si128(value);
    return result;
}

inline
__m128i
mm_unpack_low(__m128i a, __m128i b) {
    __m128i result;
    result = _mm_unpacklo_epi32(a, b);
    return result;
}

inline
__m128i
mm_unpack_high(__m128i a, __m128i b) {
    __m128i result;
    result = _mm_unpackhi_epi32(a, b);
    return result;
}

inline
__m128
mm_init_f32(f32 value) {
    __m128 result;
    result = _mm_set1_ps(value);
    return result;
}

inline
__m128
mm_init_f32(f32 a, f32 b, f32 c, f32 d) {
    __m128 result;
    result = _mm_set_ps(a,b,c,d);
    return result;
}

inline
__m128
mm_square(__m128 a) {
    __m128 result;
    result = _mm_mul_ps(a, a);
    return result;
}

inline
__m128
mm_mul(__m128 a, __m128 b) {
    __m128 result;
    result = _mm_mul_ps(a, b);
    return result;
}

inline
__m128
mm_min(__m128 a, __m128 b) {
    __m128 result;
    result = _mm_min_ps(a, b);
    return result;
}

inline
__m128
mm_max(__m128 a, __m128 b) {
    __m128 result;
    result = _mm_max_ps(a, b);
    return result;
}

inline
__m128
mm_sqrt(__m128 a) {
    __m128 result;
    result = _mm_sqrt_ps(a);
    return result;
}

inline
__m128
mm_add(__m128 a, __m128 b) {
    __m128 result;
    result = _mm_add_ps(a, b);
    return result;
}

inline
__m128
mm_sub(__m128 a, __m128 b) {
    __m128 result;
    result = _mm_sub_ps(a, b);
    return result;
}
*/
//

#endif //INTRINSICS_H
