/* date = March 1st 2025 11:42 pm */

#ifndef DEBUG_H
#define DEBUG_H

struct Debug_counter_snapshot {
  u32 hit_count;
  u64 cycle_count;
};

#define DEBUG_SNAPSHOT_COUNT 120
struct Debug_counter_state {
  char *file_name;
  char *function_name;
  
  u32 line_number;
  
  Debug_counter_snapshot snapshot_list[DEBUG_SNAPSHOT_COUNT];
};

struct Debug_state {
  u32 snapshot_index;
  u32 counter_count;
  Debug_counter_state counter_state_list[512];
  Debug_frame_end_info frame_end_info_list[DEBUG_SNAPSHOT_COUNT];
};

struct Render_group;
struct Game_asset_list;
static Render_group *debug_render_group;

static void debug_reset(Game_asset_list *asset_list, u32 width, u32 height);
static void debug_overlay(Game_memory *memory);

#endif //DEBUG_H
