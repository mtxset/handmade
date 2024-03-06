/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "utils.h"
#include "memory.h"
#include "world.h"
#include "sim_region.h"
#include "entity.h"
#include "render_group.h"
#include "asset.h"

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

#if INTERNAL
enum Debug_cycle_counter_type {
  Debug_cycle_counter_type_game_update_render,      // 0
  Debug_cycle_counter_type_render_group_to_output,  // 1
  Debug_cycle_counter_type_render_draw_rect_slow,   // 2
  Debug_cycle_counter_type_process_pixel,           // 3
  Debug_cycle_counter_type_render_draw_rect_quak,   // 4
  Debug_cycle_counter_count
};

typedef struct Debug_cycle_counter {
  u64 cycle_count;
  u32 hit_count;
} Debug_cycle_counter;
#endif

struct Platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(Platform_work_queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(Platform_work_queue_callback);

typedef void Platform_add_entry(Platform_work_queue *queue, Platform_work_queue_callback *callback, void *data);
typedef void Platform_complete_all_work(Platform_work_queue *queue);

typedef struct Game_memory {
  u64 permanent_storage_size;
  void* permanent_storage;
  
  u64 transient_storage_size;
  void* transient_storage;
  
  Platform_work_queue *high_priority_queue;
  Platform_work_queue *low_priority_queue;
  
  Platform_add_entry *platform_add_entry;
  Platform_complete_all_work *platform_complete_all_work;
  
#if INTERNAL
  Debug_cycle_counter counter_list[Debug_cycle_counter_count];
#endif
} Game_memory;

struct drop {
  bool active;
  f32 a;
  v2 pos;
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

inline
Loaded_bmp*
get_bitmap(Game_asset_list *asset_list, Bitmap_id id) {
  Loaded_bmp *result = asset_list->bitmap_list[id.value].bitmap;
  
  return result;
}

struct Hero_bitmap_ids {
  Bitmap_id head;
  Bitmap_id cape;
  Bitmap_id torso;
};

struct Game_state {
  bool is_initialized;
  Memory_arena world_arena;
  World* world;
  
  f32 typical_floor_height;
  
  World_position camera_pos;
  f32 t_sine;
  f32 t_mod;
  
  Controlled_hero controlled_hero_list[macro_array_count(((Game_input*)0)->gamepad)]; // "c"
  
  u32 low_entity_count;
  Low_entity low_entity_list[100000];
  
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
  
#if 0
  Pacman_state pacman_state;
#endif
  
  Subpixel_test subpixels;
  
  Each_monitor_pixel monitor_pixels;
  
  i32 drop_index;
  drop drops[32];
  
  bool bezier_init;
  v2 p0_offset;
  v2 p3_offset;
  
  f32 sin_cos_state;
  u32 test_sample_index;
  Loaded_sound test_sound;
};

struct Task_with_memory {
  bool being_used;
  Memory_arena arena;
  Temp_memory memory_flush;
};

struct Transient_state {
  bool is_initialized;
  Memory_arena tran_arena;
  
  Task_with_memory task_list[4];
  
  u32 ground_buffer_count;
  Ground_buffer* ground_buffer_list;
  
  Platform_work_queue *high_priority_queue;
  Platform_work_queue *low_priority_queue;
  
  u32 env_map_width;
  u32 env_map_height;
  Env_map env_map_list[3]; // 0 bottom, 1 middle, 2 is top
  
  Game_asset_list *asset_list;
};

Game_controller_input* 
get_gamepad(Game_input* input, i32 input_index) {
  macro_assert(input_index >= 0);
  macro_assert(input_index < macro_array_count(input->gamepad));
  
  return &input->gamepad[input_index];
}

typedef 
void (game_update_render_signature) (Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) (Game_memory* memory, Game_sound_buffer* sound_buffer);

internal
Low_entity* get_low_entity(Game_state* game_state, u32 index) {
  Low_entity* entity = 0;
  
  if (index > 0 && index < game_state->low_entity_count) {
    entity = game_state->low_entity_list + index;
  }
  
  return entity;
}

global_var Platform_add_entry         *platform_add_entry;
global_var Platform_complete_all_work *platform_complete_all_work;

internal Task_with_memory* begin_task_with_mem(Transient_state *tran_state);
internal void end_task_with_mem(Task_with_memory *task);

#endif //GAME_H
