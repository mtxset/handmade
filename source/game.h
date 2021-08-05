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
    i16* samples;
};

struct game_button_state {
    int half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_analog;
    f32 start_x;
    f32 start_y;
    
    f32 min_x;
    f32 min_y;
    
    f32 max_x;
    f32 max_y;
    
    f32 end_x;
    f32 end_y;
    
    union {
        game_button_state buttons[6];
        struct {
            game_button_state up;
            game_button_state down;
            game_button_state left;
            game_button_state right;
            game_button_state l1;
            game_button_state r1;
        };
    };
};

struct game_input {
    game_controller_input gamepad[4];
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
};

#endif //GAME_H
