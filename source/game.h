/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#define macro_array_count(array) sizeof(array) / sizeof((array)[0]) // array is in parenthesis because we can pass x + y and we want to have (x + y)[0]

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

struct game_button_state {
    int half_transition_count;
    bool ended_down;
};

struct game_controller_input {
    bool is_analog;
    float start_x;
    float start_y;
    
    float min_x;
    float min_y;
    
    float max_x;
    float max_y;
    
    float end_x;
    float end_y;
    
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

#endif //GAME_H
