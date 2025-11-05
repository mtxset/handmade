/* date = March 1st 2025 11:42 pm */

#ifndef DEBUG_H
#define DEBUG_H

enum Debug_var_to_text_flag {
  Debug_var_to_text_add_debug_ui = 0x1,
  Debug_var_to_text_add_name = 0x2,
  Debug_var_to_text_float_suffix = 0x4,
  Debug_var_to_text_line_end = 0x8,
  Debug_var_to_text_null_terminator = 0x10,
  Debug_var_to_text_colon = 0x20,
  Debug_var_to_text_pretty_bools = 0x40,
};

enum Debug_var_type {
  Debug_var_type_bool,
  Debug_var_type_i32,
  Debug_var_type_u32,
  Debug_var_type_f32,
  Debug_var_type_v2,
  Debug_var_type_v3,
  Debug_var_type_v4,
  
  Debug_var_type_counter_thread_list,
  Debug_var_type_bitmap_display,
  
  Debug_var_type_group,
};

struct Debug_var;

struct Debug_var_reference {
  Debug_var *var;
  Debug_var_reference *next;
  Debug_var_reference *parent;
};

struct Debug_var_group {
  bool expanded;
  Debug_var_reference *first_child;    
  Debug_var_reference *last_child;
};

struct Debug_var_hierarchy {
  v2 initial_pos;
  
  Debug_var_reference *group;
  
  Debug_var_hierarchy *next;
  Debug_var_hierarchy *prev;
};

struct Debug_profile_settings {
  v2 dimension;
};

struct Debug_bitmap_display {
  Bitmap_id id;
  v2 dim;
  bool alpha;
};

struct Debug_var {
  Debug_var_type type;
  char *name;
  
  union {
    bool bool_val;
    i32 int32;
    u32 uint32;
    f32 float32;
    v2 vec2;
    v3 vec3;
    v4 vec4;
    Debug_var_group group;
    Debug_profile_settings profile;
    Debug_bitmap_display bitmap_display;
  };
};

enum Debug_text_op {
  Debug_text_op_draw_text,
  Debug_text_op_size_text,
};

struct Debug_counter_snapshot {
  u32 hit_count;
  u64 cycle_count;
};

struct Debug_counter_state {
  char *file_name;
  char *block_name;
  
  u32 line_number;
};

struct Debug_frame_region {
  Debug_record *record;
  u64 cycle_count;
  u16 lane_index;
  u16 color_index;
  f32 min_t;
  f32 max_t;
};

#define MAX_REGIONS_PER_FRAME 2 * 4096
struct Debug_frame {
  u64 begin_clock;
  u64 end_clock;
  f32 wall_seconds_elapsed;
  
  u32 region_count;
  Debug_frame_region *region_list;
};

struct Open_debug_block {
  u32 starting_frame_index;
  Debug_record *source;
  Debug_event  *opening_event;
  
  Open_debug_block *parent;
  Open_debug_block *next_free;
};

struct Debug_thread {
  u32 id;
  u32 lane_index;
  Open_debug_block *first_open_block;
  Debug_thread *next;
};

enum Debug_interaction_type {
  Debug_interaction_none,
  
  Debug_interaction_nop,
  
  Debug_interaction_auto_modify_var,
  
  Debug_interaction_toggle,
  Debug_interaction_drag,
  Debug_interaction_tear,
  
  Debug_interaction_resize,
  Debug_interaction_move
};

struct Debug_interaction {
  Debug_interaction_type type;
  
  union {
    void *generic;
    Debug_var *var;
    Debug_var_hierarchy *hierarchy;
    v2 *pos;
  };
};

struct Debug_state {
  
  bool init;
  
  Platform_work_queue *high_priority_queue;
  
  Memory_arena debug_arena;
  
  Render_group *render_group;
  Loaded_font *debug_font;
  Hha_font *debug_font_info;
  
  bool compiling;
  Debug_executing_process compiler;
  
  v2 menu_pos;
  bool menu_active;
  
  Debug_var_reference *root_group;
  Debug_var_hierarchy hierarchy_sentinel;
  
  v2 last_mouse_pos;
  Debug_interaction interaction;
  Debug_interaction hot_interaction;
  Debug_interaction next_hot_interaction;
  
  f32 left_edge;
  f32 right_edge;
  f32 at_y;
  f32 font_scale;
  Font_id font_id;
  f32 global_width;
  f32 global_height;
  
  Debug_record *scope_to_record;
  
  Memory_arena collate_arena;
  Temp_memory  collate_temp;
  
  u32 collation_array_index;
  Debug_frame *collation_frame;
  u32 frame_bar_lane_count;
  u32 frame_count;
  f32 frame_bar_scale;
  bool paused;
  
  Rect2 profile_rect;
  bool profile_on;
  
  Debug_frame *frame_list;
  Debug_thread *first_thread;
  Open_debug_block *first_free_block;
};


static void debug_start(Game_asset_list *asset_list, u32 width, u32 height);
static void debug_end(Game_input *input, Loaded_bmp *draw_buffer);

static void refresh_collation(Debug_state *debug_state);

#endif //DEBUG_H
