/* date = July 29th 2021 7:07 pm */

#ifndef MAIN_H
#define MAIN_H

typedef signed char        i8;
typedef short              i16;
typedef int                i32;
typedef long long          i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

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

struct win32_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct win32_window_dimensions {
    int width;
    int height;
};

struct win32_sound_output {
    int samples_per_second;
    int latency_sample_count;
    f32 sine_val;
    i32 bytes_per_sample;
    i32 buffer_size;
    u32 running_sample_index;
};

#endif //MAIN_H
