/* date = August 6th 2021 2:01 am */

#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265358979323846f

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

template <typename T> void swap(T& a, T& b);
u32 truncate_u64(u64 value);
void string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest);

#endif //UTILS_H
