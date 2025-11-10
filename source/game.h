/* date = July 8th 2021 10:58 pm */
#ifndef GAME_H
#define GAME_H

#include <math.h>

#include "types.h"
#include "platform.h"
#include "memory.h"
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
#include "debug.h"

#define introspect(x)

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
bool
is_valid(Sound_id id) {
  bool result = (id.value != 0);
  
  return result;
}

struct Hero_bitmap_ids {
  Bitmap_id head;
  Bitmap_id cape;
  Bitmap_id torso;
};

struct Particle_cell {
  f32 density;
  v3 velocity_times_density;
};

struct Particle {
  Bitmap_id bitmap_id;
  v3 pos;
  v3 velocity;
  v3 acceleration;
  f32 size;
  v4 color;
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
  Loaded_bmp test_font; 
  
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
  
#define PARTICLE_CELL_DIM 16
  u32           next_particle;
  Particle      particle_list[512];
  Particle_cell particle_cell_list[PARTICLE_CELL_DIM][PARTICLE_CELL_DIM];
};

struct Task_with_memory {
  bool being_used;
  Memory_arena arena;
  Temp_memory memory_flush;
};

struct Transient_state {
  bool is_initialized;
  Memory_arena arena;
  
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

static
Low_entity* get_low_entity(Game_state* game_state, u32 index) {
  Low_entity* entity = 0;
  
  if (index > 0 && index < game_state->low_entity_count) {
    entity = game_state->low_entity_list + index;
  }
  
  return entity;
}

static Task_with_memory* begin_task_with_mem(Transient_state *tran_state);
static void end_task_with_mem(Task_with_memory *task);

static Platform_api platform;

#endif //GAME_H
