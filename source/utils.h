/* date = August 6th 2021 2:01 am */

#ifndef UTILS_H
#define UTILS_H

global_var const float PI = 3.14159265358979323846f;

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if DEBUG
#define macro_assert(expr) if (!(expr)) {*(i32*)0 = 0;}
#else
#define macro_assert(expr)
#endif 

#define macro_kilobytes(value) (value) * 1024ull
#define macro_megabytes(value) (macro_kilobytes(value) * 1024ull)
#define macro_gigabytes(value) (macro_megabytes(value) * 1024ull)
#define macro_terabytes(value) (macro_gigabytes(value) * 1024ull)
#define macro_array_count(array) (sizeof(array) / sizeof((array)[0])) // array is in parenthesis because we can pass x + y and we want to have (x + y)[0]

template <typename T> 
void swap(T& a, T& b);

void string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest);

i32 string_len(char*);
i32 string_to_binary(char*);

char* i32_to_string(i32);

i32 round_f32_i32(f32);
u32 round_f32_u32(f32);

u32 truncate_u64_u32(u64);
i32 truncate_f32_i32(f32);

i32 floor_f32_i32(f32);

i32 clamp_i32(i32 value, i32 min, i32 max);
f32 clamp_f32(f32 value, f32 min, f32 max);
#endif //UTILS_H
