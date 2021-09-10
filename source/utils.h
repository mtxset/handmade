/* date = August 6th 2021 2:01 am */

#ifndef UTILS_H
#define UTILS_H

static const float PI = 3.14159265358979323846f;

static const u32 color_black  = 0xf0000000;
static const u32 color_white  = 0xffffffff;
static const u32 color_purple = 0x00ff00ff;
static const u32 color_cyan   = 0x0000ffff;
static const u32 color_red    = 0x00ff0000;
static const u32 color_green  = 0x0000ff00;
static const u32 color_blue   = 0x000000ff;
static const u32 color_gold   = 0x00b48c06;
static const u32 color_yellow = 0x00ffff00;
static const u32 color_gray   = 0x0075787b;

#if DEBUG
#define macro_assert(expr) if (!(expr)) {*(int*)0 = 0;}
#else
#define macro_assert(expr)
#endif 

#define macro_kilobytes(value) (value)*1024ull
#define macro_megabytes(value) (macro_kilobytes(value)*1024ull)
#define macro_gigabytes(value) (macro_megabytes(value)*1024ull)
#define macro_terabytes(value) (macro_gigabytes(value)*1024ull)
#define macro_array_count(array) sizeof(array) / sizeof((array)[0]) // array is in parenthesis because we can pass x + y and we want to have (x + y)[0]

template <typename T> 
void swap(T& a, T& b);

void string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest);

int string_len(char*);
int string_to_binary(char*);

char* int_to_string(int);

inline i32 round_f32_i32(f32);
inline u32 round_f32_u32(f32);

inline u32 truncate_u64_u32(u64);
inline i32 truncate_f32_i32(f32);

inline i32 floor_f32_i32(f32);
#endif //UTILS_H
