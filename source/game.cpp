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

static void game_output_sound(game_sound_buffer* sound_buffer, int tone_hz) {
    static float t_sine;
    int16_t tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    
    auto sample_out = sound_buffer->samples;
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        float sine_val = sinf(t_sine);
        int16_t sample_value = (int16_t)(sine_val * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        t_sine += PI * 2.0f * ((float)1.0 / (float)wave_period);
    }
}

static void game_update_render(game_input* input, game_bitmap_buffer* bitmap_buffer, game_sound_buffer* sound_buffer) {
    static int blue_offset = 0;
    static int green_offset = 0;
    static int tone_hz = 256;
    
    auto input_0 = &input->gamepad[0];
    
    if (input_0->is_analog) {
        blue_offset += 4 * (int)input_0->end_x;
        tone_hz = 256 + (int)(128.0f * input_0->end_y);
    } else {
        // digital
    }
    
    if (input_0->up.ended_down) {
        green_offset += 1;
    }
    
    game_output_sound(sound_buffer, tone_hz);
    render_255_gradient(bitmap_buffer, blue_offset, green_offset);
}