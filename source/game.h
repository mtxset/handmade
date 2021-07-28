/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

struct game_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    void* memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct game_sound_buffer {
    int sample_count;
    int samples_per_second;
    int16_t* samples;
};

static void game_update_render(game_bitmap_buffer* bitamp_buffer, int blue_offset, int green_offset, game_sound_buffer* sound_buffer);

#endif //GAME_H
