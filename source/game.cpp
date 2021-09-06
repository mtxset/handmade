#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "game.h"

void 
render_255_gradient(game_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {
    
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

static 
void game_output_sound(game_sound_buffer* sound_buffer, int tone_hz, game_state* state) {
    i16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    auto sample_out = sound_buffer->samples;
    
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        f32 sine_val = sinf(state->t_sine);
        i16 sample_value = (i16)(sine_val * tone_volume);
        
        // disable sound for time being
        // sample_value = 0;
        
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        state->t_sine += PI * 2.0f * (1.0f / (f32)wave_period);
        
        
        // cuz sinf loses its floating point precision???
        if (state->t_sine > PI * 2.0f)
            state->t_sine -= PI * 2.0f;
    }
}

static 
void game_render_player(game_bitmap_buffer* backbuffer, int pos_x, int pos_y) {
    
    auto end_buffer = (u8*)backbuffer->memory + backbuffer->pitch * backbuffer->height;
    int color = 0xffffffff;
    int top = pos_y;
    int bottom = pos_y + 10;
    
    for (int x = pos_x; x < pos_x + 10; x++) {
        auto pixel = (u8*)backbuffer->memory + x * backbuffer->bytes_per_pixel + top * backbuffer->pitch;
        for (int y = pos_y; y < bottom; y++) {
            if (pixel >= backbuffer->memory && pixel <= end_buffer) {
                *(u32*)pixel = color;
            }
            pixel += backbuffer->pitch;
        }
    }
}

// this is super disgusting (no params on function)
extern "C" GAME_UPDATE_AND_RENDER(game_update_render) {
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
        state->t_sine = 0.0f;
        state->player_x = 600;
        state->player_y = 600;
        
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
        
        state->player_x += 10 * (int)input_state->stick_avg_x;
        state->player_y -= 10 * (int)input_state->stick_avg_y;
        
        
        if (state->jump_state > 0) {
            auto sin_val = sinf(0.5f * PI * state->jump_state);
            state->player_y += (int)(10.0f * sin_val);
        }
        
        if (input_state->start.ended_down) {
            state->jump_state = 4.0f;
        }
        
        state->jump_state -= 0.033f;
    }
    
    render_255_gradient(bitmap_buffer, state->blue_offset, state->green_offset);
    game_render_player(bitmap_buffer, state->player_x, state->player_y);
    
    game_render_player(bitmap_buffer, input->mouse_x, input->mouse_y);
    
    for (int i = 0; i < macro_array_count(input->mouse_buttons); i++) {
        if (input->mouse_buttons[i].ended_down)
            game_render_player(bitmap_buffer, 20 + 11 * i, 20); 
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(game_get_sound_samples) {
    auto state = (game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, state->tone_hz,  state);
}