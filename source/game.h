/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "utils.h"
#include "world.h"
#include "sim_region.h"
#include "entity.h"
#include "render_group.h"

#define BITMAP_BYTES_PER_PIXEL 4

struct Game_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
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
            Game_button_state action_up;
            Game_button_state action_down;
            Game_button_state action_left;
            Game_button_state action_right;
            
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
    
    bool executable_reloaded;
};

typedef struct Game_memory {
    bool is_initialized;
    
    u64 permanent_storage_size;
    void* permanent_storage;
    
    u64 transient_storage_size;
    void* transient_storage;
} Game_memory;

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
    
    u32 temp_count;
};

struct Temp_memory {
    Memory_arena* arena;
    size_t used;
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
    Loaded_bmp head;
    Loaded_bmp cape;
    Loaded_bmp torso;
};

struct Low_entity {
    World_position position;
    Sim_entity sim;
};

struct Add_low_entity_result {
    Low_entity* low;
    u32 low_index;
};

struct Controlled_hero {
    u32 entity_index;
    v2 dd_player;
    v2 d_sword;
    f32 d_z;
    f32 boost;
};

struct Pairwise_collision_rule {
    bool can_collide;
    u32 storage_index_a;
    u32 storage_index_b;
    
    Pairwise_collision_rule* next_in_hash;
};

struct Ground_buffer {
    World_position position;
    Loaded_bmp bitmap;
};

struct Game_state {
    Memory_arena world_arena;
    World* world;
    
    f32 typical_floor_height;
    
    World_position camera_pos;
    f32 t_sine;
    
    Controlled_hero controlled_hero_list[macro_array_count(((Game_input*)0)->gamepad)]; // @disgusting
    
    u32 low_entity_count;
    Low_entity low_entity_list[100000];
    
    Loaded_bmp grass[2];
    Loaded_bmp stone[4];
    Loaded_bmp tuft[3];
    
    Loaded_bmp tree;
    Loaded_bmp background;
    Loaded_bmp monster;
    Loaded_bmp familiar;
    Loaded_bmp sword;
    Loaded_bmp stairwell;
    u32 following_entity_index;
    
    Pairwise_collision_rule* collision_rule_hash[256];
    Pairwise_collision_rule* first_free_collsion_rule;
    
    Sim_entity_collision_group* null_collision;
    Sim_entity_collision_group* sword_collision;
    Sim_entity_collision_group* stairs_collision;
    Sim_entity_collision_group* player_collision;
    Sim_entity_collision_group* monster_collision;
    Sim_entity_collision_group* familiar_collision;
    Sim_entity_collision_group* wall_collision;
    Sim_entity_collision_group* std_room_collision;
    
    f32 time;
    
    Loaded_bmp test_diffuse;
    Loaded_bmp test_normal;
    
    Hero_bitmaps hero_bitmaps[4];
    
#if 0
    Pacman_state pacman_state;
#endif
    
    Subpixel_test subpixels;
    
    Each_monitor_pixel monitor_pixels;
    
    i32 drop_index;
    drop drops[32];
};

struct Transient_state {
    bool is_initialized;
    Memory_arena tran_arena;
    u32 ground_buffer_count;
    Ground_buffer* ground_buffer_list;
    
    u32 env_map_width;
    u32 env_map_height;
    Env_map env_map_list[3]; // 0 bottom, 1 middle, 2 is top
};

Game_controller_input* 
get_gamepad(Game_input* input, i32 input_index) {
    macro_assert(input_index >= 0);
    macro_assert(input_index < macro_array_count(input->gamepad));
    
    return &input->gamepad[input_index];
}

typedef 
void (game_update_render_signature) (thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) (thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer);

internal
Low_entity* get_low_entity(Game_state* game_state, u32 index) {
    Low_entity* entity = 0;
    
    if (index > 0 && index < game_state->low_entity_count) {
        entity = game_state->low_entity_list + index;
    }
    
    return entity;
}

#endif //GAME_H
