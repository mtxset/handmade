/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "utils.h"

struct game_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
};

struct game_sound_buffer {
    i32 sample_count;
    i32 samples_per_second;
    i16* samples;
};

struct game_button_state {
    i32 half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_connected;
    bool is_analog;
    f32 stick_avg_x;
    f32 stick_avg_y;
    
    union {
        game_button_state buttons[13];
        struct {
            game_button_state move_up;
            game_button_state move_down;
            game_button_state move_left;
            game_button_state move_right;
            
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state l1;
            game_button_state r1;
            
            game_button_state shift;
            
            game_button_state start;
            // back button has to be last entry cuz there is macro which relies on that
            // file: game.cpp function: game_update_render
            game_button_state back;
        };
    };
};

struct game_input {
    game_button_state mouse_buttons[3];
    i32 mouse_x, mouse_y;
    
    // seconds to advance over update
    f32 time_delta;
    
    // 1 - keyboard, other gamepads
    game_controller_input gamepad[5];
};

struct game_memory {
    bool is_initialized;
    u64 permanent_storage_size;
    u64 transient_storage_size;
    
    void* permanent_storage;
    void* transient_storage;
};

struct game_state {
    f32 t_sine;
    
    i32 player_tile_x;
    i32 player_tile_y;
    
    f32 player_x;
    f32 player_y;
};

// https://www.youtube.com/watch?v=es-Bou2dIdY
struct thread_context {
    i32 placeholder;
};

struct color_f32 {
    f32 r;
    f32 g;
    f32 b;
};

struct tile_map_data {
    u32* tiles;
};

struct world_map_data {
    f32 tile_side_meters;
    i32 tile_side_pixels;
    
    i32 count_x;
    i32 count_y;
    
    i32 tile_map_count_x;
    i32 tile_map_count_y;
    
    tile_map_data* tile_maps;
};

struct canonical_location {
    i32 tile_map_x;
    i32 tile_map_y;
    
    i32 tile_x;
    i32 tile_y;
    
    f32 tile_relative_x; 
    f32 tile_relative_y;
};

struct raw_location {
    i32 tile_map_x;
    i32 tile_map_y;
    
    f32 x; 
    f32 y;
};

inline 
game_controller_input* get_gamepad(game_input* input, i32 input_index) {
    macro_assert(input_index >= 0);
    macro_assert(input_index < macro_array_count(input->gamepad));
    
    return &input->gamepad[input_index];
}

void render_255_gradient(game_bitmap_buffer* bitmap_buffer, i32 blue_offset, i32 green_offset);

static 
void game_output_sound(game_sound_buffer* sound_buffer, i32 tone_hz, game_state* state);

typedef 
void (game_update_render_signature) (thread_context* thread, game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) (thread_context* thread, game_memory* memory, game_sound_buffer* sound_buffer);

#endif //GAME_H
