/* date = August 6th 2021 2:01 am */

#ifndef UTILS_H
#define UTILS_H

#if DEBUG
#define macro_assert(expr) if (!(expr)) {*(int*)0 = 0;}
#else
#define macro_assert(expr)
#endif 

#define macro_kilobytes(value) ((uint64_t)value)*1024
#define macro_megabytes(value) (macro_kilobytes(value)*1024)
#define macro_gigabytes(value) (macro_megabytes(value)*1024)
#define macro_terabytes(value) (macro_gigabytes(value)*1024)
#define macro_array_count(array) sizeof(array) / sizeof((array)[0]) // array is in parenthesis because we can pass x + y and we want to have (x + y)[0]

template <typename T> void swap(T& a, T& b);
static u32 truncate_u64(u64 value);

#endif //UTILS_H
