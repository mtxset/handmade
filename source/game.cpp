#include "file_io.h"
#include "file_io.cpp"
#include "game.h"

static void render_255_gradient(game_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {
    
    auto row = (u8*)bitmap_buffer->memory;
    
    for (int y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (u32*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
            // pixel bytes	   1  2  3  4
            // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
            // little endian
            
            u8 blue = (u8)(x + blue_offset);
            u8 green = (u8)(y + green_offset);
            
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
    static f32 t_sine;
    i16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    
    auto sample_out = sound_buffer->samples;
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        f32 sine_val = sinf(t_sine);
        i16 sample_value = (i16)(sine_val * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        t_sine += PI * 2.0f * ((f32)1.0 / (f32)wave_period);
    }
}

static void game_update_render(game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer, game_sound_buffer* sound_buffer) {
    macro_assert(sizeof(game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    auto state = (game_state*)memory->permanent_storage;
    if (!memory->is_initialized) {
        
        char* file_name = __FILE__;
        auto bitmap_read = debug_read_entire_file(file_name);
        if (bitmap_read.content) {
            debug_write_entire_file("temp.cpp", bitmap_read.bytes_read, bitmap_read.content);
            debug_free_file(bitmap_read.content);
        }
        
        state->blue_offset = 0;
        state->green_offset = 0;
        state->tone_hz = 256;
        
        memory->is_initialized = true;
    }
    
    for (int i = 0; i < macro_array_count(input->gamepad); i++) {
        auto input_state = get_gamepad(input, i);
        
        if (input_state->is_analog) {
            state->blue_offset += 4 * (int)input_state->stick_avg_x;
            state->tone_hz = 128 + (int)(64.0f * input_state->stick_avg_y);
            
            if (state->tone_hz == 0) // in case we want to set different tone_hz we accidentaly may get to 0
                state->tone_hz = 1;
        } else {
            // digital
            int offset = 1;
            
            if (input_state->up.ended_down)
                state->green_offset += offset;
            
            if (input_state->down.ended_down)
                state->green_offset -= offset;
            
            if (input_state->left.ended_down)
                state->blue_offset += offset;
            
            if (input_state->right.ended_down)
                state->blue_offset -= offset;
        }
    }
    
    game_output_sound(sound_buffer, state->tone_hz);
    render_255_gradient(bitmap_buffer, state->blue_offset, state->green_offset);
}

inline game_controller_input* get_gamepad(game_input* input, int input_index) {
    macro_assert(input_index >= 0);
    macro_assert(input_index < macro_array_count(input->gamepad));
    
    return &input->gamepad[input_index];
}