/* date = April 1st 2022 1:31 pm */

#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

#include "game.h"

struct Loaded_bmp {
  
  void* memory;
  
  v2 align_pcent;
  
  f32 width_over_height;
  
  i32 height;
  i32 width;
  
  i32 pitch;
};

struct Env_map {
  Loaded_bmp lod[4];
  f32 pos_z;
};

enum Render_group_entry_type {
  Render_group_entry_type_Render_entry_clear,
  Render_group_entry_type_Render_entry_bitmap,
  Render_group_entry_type_Render_entry_rect,
  Render_group_entry_type_Render_entry_coord_system,
  Render_group_entry_type_Render_entry_saturation,
};

struct Render_group_entry_header {
  Render_group_entry_type type;
};

struct Render_entry_coord_system {
  v2 origin;
  v2 x_axis;
  v2 y_axis;
  v4 color;
  
  Loaded_bmp* bitmap;
  Loaded_bmp* normal_map;
  
  //f32 meters_to_pixels;
  
  Env_map* top;
  Env_map* middle;
  Env_map* bottom;
};

struct Render_entry_clear {
  v4 color;
};

struct Render_entry_saturation {
  f32 level;
};

struct Render_entry_rect {
  v2 pos;
  v2 dim;
  v4 color;
};

struct Render_entry_bitmap {
  Loaded_bmp* bitmap;
  v2 pos;
  v2 size;
  v4 color;
};

struct Render_transform {
  
  bool orthographic;
  
  f32 meters_to_pixels;
  v2 screen_center;
  
  f32 focal_length;
  f32 dist_above_target;
  
  v3 offset_pos;
  f32 scale;
};

struct Render_group {
  
  struct Game_asset_list *asset_list; 
  f32 global_alpha;
  
  u32 generation_id;
  
  v2 monitor_half_dim_meters;
  Render_transform transform;
  
  u32 max_push_buffer_size;
  u32 push_buffer_size;
  u8* push_buffer_base;
  
  u32 missing_resource_count;
  bool renders_in_background;
  
  bool inside_render;
};


struct Entity_basis_result {
  v2 pos;
  f32 scale;
  bool valid;
};

struct Used_bitmap_dim {
  Entity_basis_result basis;
  v2 size;
  v2 align;
  v3 pos;
};


#endif //RENDER_GROUP_H
void
draw_rect_quak(Loaded_bmp *buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp *bitmap, f32 pixel_to_meter, Rect2i clip_rect, bool even);