/* date = July 29th 2021 7:07 pm */

#ifndef MAIN_H
#define MAIN_H

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
    float sine_val;
    int32_t bytes_per_sample;
    int32_t buffer_size;
    uint32_t running_sample_index;
};

#endif //MAIN_H
