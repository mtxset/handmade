/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "utils.h"
#include "tile.h"

struct Game_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
    u32 window_width;
    u32 window_height;
};

struct Game_sound_buffer {
    i32 sample_count;
    i32 samples_per_second;
    i16* samples;
};

struct Game_button_state {
    i32 half_transition_count;
    bool ended_down;
};

struct Game_controller_input {
    bool is_connected;
    bool is_analog;
    f32 stick_avg_x;
    f32 stick_avg_y;
    
    union {
        Game_button_state buttons[16];
        struct {
            Game_button_state move_up;
            Game_button_state move_down;
            Game_button_state move_left;
            Game_button_state move_right;
            
            Game_button_state up;
            Game_button_state down;
            Game_button_state left;
            Game_button_state right;
            
            Game_button_state cross_or_a;
            Game_button_state circle_or_b;
            Game_button_state triangle_or_y;
            Game_button_state box_or_x;
            
            Game_button_state shift;
            Game_button_state action;
            
            Game_button_state start;
            // if assert fails increase buttons[x] by amount of new buttons, so total matches count of buttons in this struct
            // back button has to be last entry cuz there is macro which relies on that
            // file: game.cpp function: game_update_render
            Game_button_state back;
        };
    };
};

struct Game_input {
    Game_button_state mouse_buttons[3];
    i32 mouse_x, mouse_y;
    
    // seconds to advance over update
    f32 time_delta;
    
    // 1 - keyboard, other gamepads
    Game_controller_input gamepad[5];
};

struct Game_memory {
    bool is_initialized;
    u64 permanent_storage_size;
    u64 transient_storage_size;
    
    void* permanent_storage;
    void* transient_storage;
};

// https://www.youtube.com/watch?v=es-Bou2dIdY
struct thread_context {
    i32 placeholder;
};

struct drop {
    bool active;
    f32 a;
    v2 pos;
};

struct Memory_arena {
    u8* base;
    size_t size;
    size_t used;
};

struct World {
    Tile_map* tile_map;
};

struct Pacman_state {
    i32 ghost_tile_x;
    i32 ghost_tile_y;
    f32 ghost_move_timer;
    bool ghost_can_move;
    i32 ghost_direction_x;
    i32 ghost_direction_y;
    
    i32 player_tile_x;
    i32 player_tile_y;
    f32 move_timer;
    bool can_move;
};

struct Each_monitor_pixel {
    f32 x, y;
    f32 timer;
};

struct Subpixel_test {
    f32 pixel_timer;
    f32 transition_state;
    bool direction;
};

struct Loaded_bmp {
    i32 height;
    i32 width;
    u32* pixels;
};

// struct is from: https://www.fileformat.info/format/bmp/egff.htm
// preventing compiler padding this struct
#pragma pack(push, 1)
struct Bitmap_header {
    u16 FileType;        /* File type, always 4D42h ("BM") */
    u32 FileSize;        /* Size of the file in bytes */
    u16 Reserved1;       /* Always 0 */
    u16 Reserved2;       /* Always 0 */
    u32 BitmapOffset;    /* Starting position of image data in bytes */
    
    u32 Size;            /* Size of this header in bytes */
    i32 Width;           /* Image width in pixels */
    i32 Height;          /* Image height in pixels */
    u16 Planes;          /* Number of color planes */
    u16 BitsPerPixel;    /* Number of bits per pixel */
    u32 Compression;     /* Compression methods used */
    u32 SizeOfBitmap;    /* Size of bitmap in bytes */
    i32 HorzResolution;  /* Horizontal resolution in pixels per meter */
    i32 VertResolution;  /* Vertical resolution in pixels per meter */
    u32 ColorsUsed;      /* Number of colors in the image */
    u32 ColorsImportant; /* Minimum number of important colors */
    
    u32 RedMask;         /* Mask identifying bits of red component */
    u32 GreenMask;       /* Mask identifying bits of green component */
    u32 BlueMask;        /* Mask identifying bits of blue component */
};
#pragma pack(pop)

struct Hero_bitmaps {
    v2 align; // starting pointx for drawing that bitmap
    Loaded_bmp hero_body;
};

enum Entity_type {
    Entity_type_null,
    Entity_type_hero,
    Entity_type_wall
};

enum Entity_residence {
    Entity_residence_none,
    Entity_residence_dormant,
    Entity_residence_high, 
    Entity_residence_low, 
};

struct High_entity {
    v2 position;
    v2 velocity_d;
    u32 abs_tile_z;
    u32 facing_direction;
    
    f32 z;
    f32 z_velocity_d;
};

struct Low_entity {
    
};

struct Dormant_entity {
    Entity_type type;
    
    Tile_map_position position;
    f32 width, height;
    
    bool collides;
    i32 d_abs_tile_z;
};

struct Entity {
    u32 residence;
    Dormant_entity* dormant;
    Low_entity*     low;
    High_entity*    high;
};

struct Game_state {
    Memory_arena world_arena;
    World* world;
    Tile_map_position camera_pos;
    f32 t_sine;
    
    u32 player_index_for_controller[macro_array_count(((Game_input*)0)->gamepad)]; // @disgusting
    
    u32 entity_count;
    Entity_residence entity_residence[256];
    High_entity      high_entity_list[256];
    Low_entity       low_entity_list[256];
    Dormant_entity   dormant_entity_list[256];
    
    Loaded_bmp background;
    
    u32 following_entity_index;
    
    Hero_bitmaps hero_bitmaps[4];
#if 0
    Pacman_state pacman_state;
#endif
    
    Subpixel_test subpixels;
    
    Each_monitor_pixel monitor_pixels;
    
    i32 drop_index;
    drop drops[32];
};

Game_controller_input* get_gamepad(Game_input* input, i32 input_index) {
    macro_assert(input_index >= 0);
    macro_assert(input_index < macro_array_count(input->gamepad));
    
    return &input->gamepad[input_index];
}

void render_255_gradient(Game_bitmap_buffer* bitmap_buffer, i32 blue_offset, i32 green_offset);

internal
void game_output_sound(Game_sound_buffer* sound_buffer, i32 tone_hz, Game_state* state);

typedef 
void (game_update_render_signature) (thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) (thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer);

#endif //GAME_H
