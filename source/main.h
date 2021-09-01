/* date = July 29th 2021 7:07 pm */

#ifndef MAIN_H
#define MAIN_H

#include "types.h"

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
    u32 buffer_size;
    i32 safety_bytes;
    u32 running_sample_index;
};

struct win32_debug_time_marker {
    u32 output_play_cursor;
    u32 play_cursor;
    
    u32 output_write_cursor;
    u32 write_cursor;
    
    u32 output_location;
    u32 output_bytes;
    
    u32 expected_play_cursor;
};

#endif //MAIN_H
