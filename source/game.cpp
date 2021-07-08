#include "game.h"

static void render_255_gradient(game_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {
    
    auto row = (uint8_t*)bitmap_buffer->memory;
    
    for (int y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (uint32_t*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
            // pixel bytes	   1  2  3  4
            // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
            // little endian
            
            uint8_t blue = x + blue_offset;
            uint8_t green = y + green_offset;
            
            // 0x 00 00 00 00 -> 0x xx rr gg bb
            // | composites bytes
            // green << 8 - shifts by 8 bits
            // other stay 00
            // * dereference pixel
            // pixel++ - pointer arithmetic - jumps by it's size (32 bits in this case)
            *pixel++ = (green << 8) | blue;
        }
        
        row += bitmap_buffer->pitch;
    }
}

static void game_update_render(game_bitmap_buffer* buffer, int x_offset, int y_offset) {
    render_255_gradient(buffer, x_offset, y_offset);
}