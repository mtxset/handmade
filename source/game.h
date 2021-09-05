/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "utils.h"

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
    i16* samples;
};

struct game_button_state {
    int half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_connected;
    bool is_analog;
    f32 stick_avg_x;
    f32 stick_avg_y;
    
    union {
        game_button_state buttons[12];
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
            
            game_button_state start;
            game_button_state back;
        };
    };
};

struct game_input {
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
    int tone_hz;
    int blue_offset;
    int green_offset;
    f32 t_sine;
    
    int player_x;
    int player_y;
    f32 jump_state;
};

inline game_controller_input* get_gamepad(game_input* input, int input_index) {
    macro_assert(input_index >= 0);
    macro_assert(input_index < macro_array_count(input->gamepad));
    
    return &input->gamepad[input_index];
}

void render_255_gradient(game_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset);
static void game_output_sound(game_sound_buffer* sound_buffer, int tone_hz, game_state* state);

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_render_def);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory* memory, game_sound_buffer* sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples_def);

//void game_update_render(game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer);
//void game_get_sound_samples(game_memory* memory, game_sound_buffer* sound_buffer);

#endif //GAME_H
