/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include <math.h>

#include "types.h"
#include "platform.h"
#include "memory.h"
#include "file_io.h"
#include "file_formats.h"

#include "intrinsics.h"
#include "math.h"
#include "world.h"
#include "sim_region.h"
#include "entity.h"
#include "render_group.h"
#include "asset.h"
#include "random.h"
#include "audio.h"

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
Loaded_sound*
get_sound(Game_asset_list *asset_list, Sound_id id) {
  Loaded_sound *result = asset_list->slot_list[id.value].sound;
  
  return result;
}

inline
bool
is_valid(Sound_id id) {
  bool result = (id.value != 0);
  
  return result;
}

inline
Asset_sound_info*
get_sound_info(Game_asset_list *asset_list, Sound_id id) {
  assert(id.value <= asset_list->asset_count);
  
  Asset_sound_info *result = &asset_list->asset_list[id.value].sound;
  
  return result;
}

inline
Loaded_bmp*
get_bitmap(Game_asset_list *asset_list, Bitmap_id id) {
  assert(id.value <= asset_list->asset_count);
  Loaded_bmp *result = asset_list->slot_list[id.value].bitmap;
  
  return result;
}

struct Hero_bitmap_ids {
  Bitmap_id head;
  Bitmap_id cape;
  Bitmap_id torso;
};

struct Game_state {
  bool is_initialized;
  
  Memory_arena meta_arena;
  Memory_arena world_arena;
  
  World* world;
  
  f32 typical_floor_height;
  
  World_position camera_pos;
  f32 t_sine;
  f32 t_mod;
  
  Controlled_hero controlled_hero_list[array_count(((Game_input*)0)->gamepad)]; // "c"
  
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
  
  Random_series general_entropy;
  
  Audio_state audio_state;
  Playing_sound *music;
  
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
