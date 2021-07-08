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
// 
void game_update_render(game_bitmap_buffer* buffer, int blue_offset, int green_offset);

#endif //GAME_H
