#if !INTERNAL
Game_memory *debug_global_memory = 0;
#endif

#include "debug_variables.h"

inline
Debug_state*
debug_get_state(Game_memory *memory) {
  Debug_state *debug_state = (Debug_state*)memory->debug_storage;
  assert(debug_state->init);
  
  return debug_state;
}

inline
Debug_state*
debug_get_state() {
  Debug_state *result = debug_get_state(debug_global_memory);
  
  return result;
}

static
Rect2
debug_text_op(Debug_state *debug_state, Debug_text_op text_op, v2 pos, char* string, v4 color = white_v4) {
  
  Rect2 result = rect2_inverted_infinity();
  
  if (!debug_state || !debug_state->debug_font)
    goto end_of_function;
  
  Render_group *render_group = debug_state->render_group;
  Loaded_font  *font = debug_state->debug_font;
  Hha_font     *info = debug_state->debug_font_info;
  
  u32 prev_code_point = 0;
  f32 char_scale = debug_state->font_scale;
  f32 at_y = pos.y;
  f32 at_x = pos.x;
  
  for (char *at = string; *at; ) {
    if (at[0] == '\\' &&
        at[1] == '#' &&
        at[2] != 0 &&
        at[3] != 0 &&
        at[4] != 0) {
      
      f32 scale = 1.0f / 9.0f;
      
      color = v4 {
        clamp01(scale * (f32)(at[2] - '0')),
        clamp01(scale * (f32)(at[3] - '0')),
        clamp01(scale * (f32)(at[4] - '0')),
        1.0f
      };
      
      at += 5;
    }
    else if (at[0] == '\\' &&
             at[1] == '^' &&
             at[2] != 0) {
      f32 scale = 1.0f / 9.0f;
      char_scale = debug_state->font_scale * clamp01(scale * (f32)(at[2] - '0'));
      at += 3;
    }
    else {
      
      u32 code_point = *at;
      
      if(at[0] == '\\' &&
         is_hex(at[1]) &&
         is_hex(at[2]) &&
         is_hex(at[3]) &&
         is_hex(at[4])) {
        code_point= ((get_hex(at[1]) << 12) |
                     (get_hex(at[2]) << 8) |
                     (get_hex(at[3]) << 4) |
                     (get_hex(at[4]) << 0));
        at += 4;
      }
      
      f32 advance_x = char_scale * get_horizontal_advance_for_pair(info, font, prev_code_point, code_point);
      at_x += advance_x;
      
      if (code_point != ' ') {
        
        Bitmap_id bitmap_id = get_bitmap_for_glyph(render_group->asset_list, info, font, code_point);
        Hha_bitmap *bitmap_info = get_bitmap_info(render_group->asset_list, bitmap_id);
        
        f32 bitmap_scale = char_scale * (f32)bitmap_info->dim[1];
        v3 bitmap_offset = {at_x, at_y, 0};
        
        if (text_op == Debug_text_op_draw_text) {
          push_bitmap(render_group, bitmap_id, bitmap_scale, v3{at_x, at_y, 0}, color);
        }
        else if (text_op == Debug_text_op_size_text) {
          Loaded_bmp *bitmap = get_bitmap(render_group->asset_list, bitmap_id, render_group->generation_id);
          
          if (bitmap) {
            
            Used_bitmap_dim dim = get_bitmap_dim(render_group, bitmap, bitmap_scale, bitmap_offset);
            Rect2 glyph_dim = rect_min_dim(dim.pos.xy, dim.size);
            
            result = rect2_union(result, glyph_dim);
          }
          
        }
        
      }
      
      prev_code_point = code_point;
      at++;
    }
  }
  
  end_of_function:
  return result;
}

static 
void
debug_text_out_at(v2 pos, char* string, v4 color = white_v4) {
  Debug_state *debug_state = debug_get_state();
  
  if (debug_state) {
    Render_group *render_group = debug_state->render_group;
    debug_text_op(debug_state, Debug_text_op_draw_text, pos, string, color);
  }
  
}

static
void
debug_text_line(char *string) {
  
  Debug_state *debug_state = debug_get_state();
  
  if (!debug_state)
    return;
  
  Render_group *render_group = debug_state->render_group;
  
  Loaded_font *font = push_font(render_group, debug_state->font_id);
  
  if (!font)
    return;
  
  Hha_font *info = get_font_info(render_group->asset_list, debug_state->font_id);
  
  v2 text_pos = v2{
    debug_state->left_edge,
    debug_state->at_y - debug_state->font_scale * get_starting_baseline_y(debug_state->debug_font_info)
  };
  debug_text_out_at(text_pos, string);
  debug_state->at_y -= get_line_advance_for(info) * debug_state->font_scale;
}

struct Debug_stats {
  f64 min;
  f64 max;
  f64 avg;
  u32 count;
};

inline
void
begin_debug_stat(Debug_stats *stat) {
  stat->min = FLT_MAX;
  stat->max = -FLT_MAX;
  stat->avg = 0.0f;
  stat->count = 0;
}


inline
void
close_debug_stat(Debug_stats *stat) {
  
  if (stat->count) {
    stat->avg /= (f64)stat->count;
  }
  else {
    stat->min = 0.0f;
    stat->max = 0.0f;
  }
}

inline
void
accumulate_debug_stats(Debug_stats *stat, f64 value) {
  stat->count++;
  stat->avg += value;
  
  if (stat->min > value) stat->min = value;
  if (stat->max < value) stat->max = value;
}

static
void
restart_collation(Debug_state *debug_state, u32 invalid_event_array_index) {
  end_temp_memory(debug_state->collate_temp);
  
  debug_state->collate_temp = begin_temp_memory(&debug_state->collate_arena);
  
  debug_state->first_thread = 0;
  debug_state->first_free_block = 0;
  
  debug_state->frame_list = mem_push_array(&debug_state->collate_arena, MAX_DEBUG_EVENT_ARRAY_COUNT * 4, Debug_frame);
  debug_state->frame_bar_lane_count = 0;
  debug_state->frame_count = 1;
  debug_state->frame_bar_scale = 1.0f / 60000000.0f; ////?????
  debug_state->collation_array_index = invalid_event_array_index + 1;
  debug_state->collation_frame = 0;
}

static
void
debug_start(Game_asset_list *asset_list, u32 width, u32 height) {
  
  timed_function();
  
  Debug_state *debug_state = (Debug_state*)debug_global_memory->debug_storage;
  
  if (debug_state) { 
    
    if (!debug_state->init) {
      
      debug_state->high_priority_queue = debug_global_memory->high_priority_queue;
      
      initialize_arena(&debug_state->debug_arena, debug_global_memory->debug_storage_size - sizeof(Debug_state), debug_state + 1);
      
      {
        Debug_var_defintion_context debug_ctx = {};
        debug_ctx.state = debug_state;
        debug_ctx.arena = &debug_state->debug_arena;
        debug_ctx.group = debug_begin_variable_group(&debug_ctx, "root");
        
        debug_begin_variable_group(&debug_ctx, "debugging");
        debug_create_vars(&debug_ctx);
        
        debug_begin_variable_group(&debug_ctx, "profile");
        {
          {
            debug_begin_variable_group(&debug_ctx, "by thread");
            Debug_var *thread_list = debug_add_var(&debug_ctx, Debug_var_type_counter_thread_list, "");
            thread_list->profile.dimension = V2(1024, 100);
            debug_end_variable_group(&debug_ctx);
          }
          
          {
            debug_begin_variable_group(&debug_ctx, "by function");
            Debug_var *function_list = debug_add_var(&debug_ctx, Debug_var_type_counter_thread_list, "");
            function_list->profile.dimension = V2(1024, 200);
            debug_end_variable_group(&debug_ctx);
          }
          debug_end_variable_group(&debug_ctx);
        }
        
        debug_end_variable_group(&debug_ctx);
        
        debug_state->root_group = debug_ctx.group;
      }
      
      debug_state->render_group = allocate_render_group(asset_list, &debug_state->debug_arena, megabytes(16), false);
      
      debug_state->paused = false;
      debug_state->scope_to_record = 0;
      debug_state->init = true;
      
      sub_arena(&debug_state->collate_arena, &debug_state->debug_arena, megabytes(32), 4);
      debug_state->collate_temp = begin_temp_memory(&debug_state->collate_arena);
      
      restart_collation(debug_state, 0);
      
    }
    
    begin_render(debug_state->render_group);
    debug_state->debug_font = push_font(debug_state->render_group, debug_state->font_id);
    debug_state->debug_font_info = get_font_info(debug_state->render_group->asset_list, debug_state->font_id);
    
    debug_state->global_width  = (f32)width;
    debug_state->global_height = (f32)height;
    
    Asset_vector match_vector = {};
    Asset_vector weight_vector = {};
    match_vector.e[Tag_font_type] = (f32)Font_type_debug;
    weight_vector.e[Tag_font_type] = 1.0f;
    
    debug_state->font_id = get_best_match_font_from(asset_list, Asset_font, &match_vector, &weight_vector);
    
    debug_state->font_scale = 1.0f;
    ortographic(debug_state->render_group, width, height, 1.0f);
    debug_state->left_edge = -.5f * width;
    
    Hha_font *info = get_font_info(asset_list, debug_state->font_id);
    debug_state->at_y = 0.5f * height;
  }
  
  debug_state->hierarchy.group = debug_state->root_group;
  debug_state->hierarchy.initial_pos = V2(debug_state->left_edge, debug_state->at_y);
}

static
Rect2
debug_get_text_size(Debug_state *debug_state, char *string) {
  Rect2 result = debug_text_op(debug_state, Debug_text_op_size_text, v2_zero, string);
  
  return result;
}

static
size_t
debug_var_to_text(char *buffer, char *end, Debug_var *var, u32 flags) {
  
  char *at = buffer;
  
  if (flags & Debug_var_to_text_add_debug_ui) {
    at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "#define DEBUG_UI_");
  }
  
  if (flags & Debug_var_to_text_add_name) {
    at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), 
                      "%s%s ", var->name, (flags & Debug_var_to_text_colon) ? ":" : "");
  }
  
  switch (var->type) {
    
    case Debug_var_type_bool: {
      
      if (flags & Debug_var_to_text_pretty_bools) {
        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                          "%s",
                          var->bool_val ? "true" : "false");
      }
      else {
        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                          "%d", var->bool_val);
      }
    } break;
    
    case Debug_var_type_i32: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "%d", var->int32);
    } break;
    
    case Debug_var_type_u32: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "%u", var->uint32);
    } break;
    
    case Debug_var_type_v2: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "V2(%f, %f)", var->vec2.x, var->vec2.y);
    } break;
    
    case Debug_var_type_v3: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "V3(%f, %f, %f)", var->vec3.x, var->vec3.y, var->vec3.z);
    } break;
    
    case Debug_var_type_v4: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "V4(%f, %f, %f, %f)", var->vec4.x, var->vec4.y, var->vec4.z, var->vec4.w);
    } break;
    
    case Debug_var_type_f32: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at),
                        "%f", var->float32);
      
      if(flags & Debug_var_to_text_float_suffix) {
        *at++ = 'f';
      }
    } break;
    
    case Debug_var_type_group: {
    } break;
    
    default: assert(!"qe?");
  }
  
  if (flags & Debug_var_to_text_line_end) {
    *at++ = '\n';
  }
  
  if (flags & Debug_var_to_text_null_terminator) {
    *at++ = 0;
  }
  
  return (at - buffer);
}

inline
bool
debug_should_be_written(Debug_var_type type) {
  bool result = (type != Debug_var_type_counter_thread_list);
  
  return result;
}

static
void
write_debug_config(Debug_state *debug_state) {
  
  char temp[4096];
  char *at = temp;
  char *end = temp + sizeof(temp);
  
  i32 depth = 0;
  Debug_var *var = debug_state->root_group->group.first_child;
  
  while (var) {
    
    if (debug_should_be_written(var->type)) {
      
      for (i32 ident = 0; ident < depth; ident++) {
        *at++ = ' '; *at++ = ' '; *at++ = ' '; *at++ = ' ';
      }
      
      
      if (var->type == Debug_var_type_group) {
        at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "// ");
      }
      
      
      at += debug_var_to_text(at, end, var,
                              Debug_var_to_text_add_debug_ui |
                              Debug_var_to_text_add_name |
                              Debug_var_to_text_line_end |
                              Debug_var_to_text_float_suffix |
                              Debug_var_to_text_pretty_bools);
      
    }
    
    if (var->type == Debug_var_type_group) {
      var = var->group.first_child;
      depth++;
    }
    else {
      
      
      while (var) {
        
        if (var->next) {
          var = var->next;
          break;
        }
        else {
          var = var->parent;
          depth--;
        }
        
      }
      
      
    }
  }
  
  platform.debug_write_entire_file("..\\source\\config.h", (u32)(at - temp), temp);
  
  if (!debug_state->compiling) {
    debug_state->compiling = true;
    
    debug_state->compiler = platform.debug_execute_cmd("..", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
  }
  
}

static
void
draw_profile_in(Debug_state *debug_state, Rect2 profile_rect, v2 mouse_pos) {
  
  push_rect(debug_state->render_group, profile_rect, 0.0f, v4{0,0,0, .25f});
  
  f32 bar_spacing = 10.0f;
  f32 lane_height = 0.0f;
  u32 lane_count  = debug_state->frame_bar_lane_count;
  
  u32 max_frame = debug_state->frame_count;
  if (max_frame > 10) max_frame = 10;
  
  if (lane_count > 0 && max_frame > 0) {
    f32 pixels_per_frame_plus_spacing = get_dim(profile_rect).y / (f32)max_frame;
    f32 pixels_per_frame = pixels_per_frame_plus_spacing - bar_spacing;
    lane_height = pixels_per_frame / (f32)lane_count;
  }
  
  f32 bar_height = lane_height * lane_count;
  f32 bar_plus_spacing = bar_height + bar_spacing;
  f32 chart_left = profile_rect.min.x;
  f32 chart_height = bar_plus_spacing * (f32)max_frame;
  f32 chart_width  = get_dim(profile_rect).x;
  f32 chart_top = profile_rect.max.y;
  f32 scale = chart_width * debug_state->frame_bar_scale;
  
  v3 color_list[] = {
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 1},
    {1, 0.5f, 0},
    {1, 0, 0.5f},
    {0.5f, 1, 0},
    {0, 1, 0.5f},
    {0.5f, 0, 1},
    {0, 0.5f, 1},
  };
  
  Render_group *render_group = debug_state->render_group;
  
  for (u32 frame_index = 0; frame_index < max_frame; frame_index += 1) {
    Debug_frame *frame = debug_state->frame_list + debug_state->frame_count - (frame_index + 1);
    
    f32 stack_x = chart_left;
    f32 stack_y = chart_top - bar_plus_spacing * (f32)frame_index;
    
    for (u32 region_index = 0; region_index < frame->region_count; region_index += 1) {
      Debug_frame_region *region = frame->region_list + region_index;
      
      v3 color = color_list[region->color_index % array_count(color_list)];
      f32 this_min_x = stack_x + scale * region->min_t;
      f32 this_max_x = stack_x + scale * region->max_t;
      
      Rect2 rect = rect_min_max(v2{this_min_x, stack_y - lane_height * (region->lane_index + 1)},
                                v2{this_max_x, stack_y - lane_height * region->lane_index});
      
      push_rect(render_group, rect, .0f, V4(color, 1));
      
      if (is_in_rect(rect, mouse_pos)) {
        Debug_record *record = region->record;
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "%s: %10Iucy [%s(%d)]",
                    record->block_name,
                    region->cycle_count,
                    record->file_name,
                    record->line_number);
        
        debug_text_out_at(mouse_pos + v2{0, 10.0f}, buffer);
      }
      
    }
  }
}

static
void
debug_draw_main_menu(Debug_state *debug_state, Render_group *render_group, v2 mouse_pos) {
  
  f32 at_x = debug_state->hierarchy.initial_pos.x;
  f32 at_y = debug_state->hierarchy.initial_pos.y;
  f32 line_advance = get_line_advance_for(debug_state->debug_font_info);
  
  f32 spacing_y = 4.0f;
  i32 depth = 0;
  Debug_var *var = debug_state->hierarchy.group->group.first_child;
  
  while (var) {
    
    bool is_hot = (debug_state->hot == var);
    v4 item_color = (is_hot && debug_state->hot_interaction == 0) ? yellow_v4 : white_v4;
    
    Rect2 bounds = {};
    
    switch (var->type) {
      
      case (Debug_var_type_counter_thread_list): {
        
        v2 min_corner = v2{
          at_x + depth * 2.0f * line_advance, 
          at_y - var->profile.dimension.y
        };
        
        v2 max_corner = v2{
          min_corner.x + var->profile.dimension.x,
          at_y
        };
        v2 size = V2(max_corner.x, min_corner.y);
        bounds = rect_min_max(min_corner, max_corner);
        draw_profile_in(debug_state, bounds, mouse_pos);
        
        Rect2 size_box = rect_center_half_dim(size, V2(4, 4));
        v4 color = (is_hot && debug_state->hot_interaction == Debug_interaction_resize_profile) ? yellow_v4 : white_v4;
        push_rect(debug_state->render_group, size_box, 0, color);
        
        if (is_in_rect(size_box, mouse_pos)) {
          debug_state->next_hot_interaction = Debug_interaction_resize_profile;
          debug_state->next_hot = var;
        }
        else if (is_in_rect(bounds, mouse_pos)) {
          debug_state->next_hot_interaction = Debug_interaction_none;
          debug_state->next_hot = var;
        }
        
      } break;
      
      default: {
        
        char text[256];
        
        debug_var_to_text(text, text + sizeof(text), var,
                          Debug_var_to_text_add_name |
                          Debug_var_to_text_null_terminator |
                          Debug_var_to_text_colon |
                          Debug_var_to_text_pretty_bools);
        
        f32 left_px = at_x + depth * 2.0f * line_advance;
        f32 top_pos_y = at_y;
        
        bounds = debug_get_text_size(debug_state, text);
        bounds = offset(bounds, V2(left_px, top_pos_y - get_dim(bounds).y));
        v2 text_pos = {
          left_px,
          top_pos_y - debug_state->font_scale * get_starting_baseline_y(debug_state->debug_font_info)
        };
        debug_text_out_at(text_pos, text, item_color);
        
        if (is_in_rect(bounds, mouse_pos)) {
          debug_state->next_hot_interaction = Debug_interaction_none;
          debug_state->next_hot = var;
        }
        
      } break;
      
    }
    
    at_y = get_min_corner(bounds).y - spacing_y;
    
    if (var->type == Debug_var_type_group && var->group.expanded) {
      var = var->group.first_child;
      depth++;
    }
    else {
      
      while (var) {
        
        if (var->next) {
          var = var->next;
          break;
        }
        else {
          var = var->parent;
          depth--;
        }
        
      }
      
    }
    
    
  }
  
  debug_state->at_y = at_y;
  
#if 0
  
  switch (var->type) {
    case Debug_var_type_bool: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "#define DEBUG_UI_%s %d\n", var->name, var->bool_val);
    } break;
    
    case Debug_var_type_group: {
      at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "// %s\n", var->name);
    } break;
    
  }
  
  u32 new_hot_menu_index = array_count(debug_var_list);
  f32 best_distance_sq = FLT_MAX;
  
  f32 menu_radius = 300.0f;
  f32 angle_step = TAU / (f32)array_count(debug_var_list);
  
  for (u32 menu_item_index = 0; menu_item_index < array_count(debug_var_list); menu_item_index++) {
    Debug_var *var = debug_var_list + menu_item_index;
    char *text = var->name;
    
    v4 item_color = var->value ? white_v4 : V4(0.5f, 0.5f, 0.5f, 1.0f);
    if (menu_item_index == debug_state->hot_menu_index) {
      item_color = yellow_v4;
    }
    
    f32 angle = (f32)menu_item_index * angle_step;
    v2 text_pos = debug_state->menu_pos + menu_radius * arm(angle);
    
    f32 this_distance_sq = length_squared(text_pos - mouse_pos);
    if (best_distance_sq > this_distance_sq) {
      new_hot_menu_index = menu_item_index;
      best_distance_sq = this_distance_sq;
    }
    
    Rect2 text_rect = debug_get_text_size(debug_state, text);
    debug_text_out_at(text_pos - .5f * get_dim(text_rect), text, item_color);
  }
  
  if (length_squared(mouse_pos - debug_state->menu_pos) > square(menu_radius)) {
    debug_state->hot_menu_index = new_hot_menu_index;
  }
  else {
    debug_state->hot_menu_index = array_count(debug_var_list);
  }
#endif
}

static
void
debug_begin_interact(Debug_state *debug_state, Game_input *input, v2 mouse_pos) {
  
  if (debug_state->hot) {
    
    if (debug_state->hot_interaction) {
      debug_state->interaction = debug_state->hot_interaction;
    }
    else {
      
      switch (debug_state->hot->type) {
        
        case Debug_var_type_bool: debug_state->interaction = Debug_interaction_toggle; break;
        case Debug_var_type_f32: debug_state->interaction = Debug_interaction_drag; break;
        case Debug_var_type_group: debug_state->interaction = Debug_interaction_toggle; break;
        
      }
    }
    
    if (debug_state->interaction) {
      debug_state->interacting_with = debug_state->hot;
    }
    
  }
  else {
    debug_state->interaction = Debug_interaction_nop;
  }
  
  
}

static
void
debug_end_interact(Debug_state *debug_state, Game_input *input, v2 mouse_pos) {
  
  if (debug_state->interaction != Debug_interaction_nop) {
    
    Debug_var *var = debug_state->interacting_with;
    
    assert(var);
    
    switch (debug_state->interaction) {
      case Debug_interaction_toggle: {
        
        switch (var->type) {
          case Debug_var_type_bool: var->bool_val = !var->bool_val; break;
          case Debug_var_type_group: var->group.expanded = !var->group.expanded; break;
        }
        
      } break;
      
      case Debug_interaction_tear: {
        
      } break;
    }
    
    write_debug_config(debug_state);
  }
  
  debug_state->interaction = Debug_interaction_none;
  debug_state->interacting_with = 0;
}

static
void
debug_interact(Debug_state *debug_state, Game_input *input, v2 mouse_pos) {
  
  v2 mouse_delta = mouse_pos - debug_state->last_mouse_pos;
  
  if (debug_state->interaction) {
    Debug_var *var = debug_state->interacting_with;
    
    switch (debug_state->interaction) {
      
      case Debug_interaction_drag: {
        
        switch (var->type) {
          case Debug_var_type_f32: var->float32 += 0.1f * mouse_delta.y; break;
        }
        
      } break;
      
      case Debug_interaction_resize_profile: { 
        var->profile.dimension += V2(mouse_delta.x, -mouse_delta.y);
        var->profile.dimension.x = max(var->profile.dimension.x, 10);
        var->profile.dimension.y = max(var->profile.dimension.y, 10);
      } break;
      
    }
    
    for (u32 transition_index = input->mouse_buttons[Game_input_mouse_button_left].half_transition_count;
         transition_index > 1; transition_index--) {
      
      debug_end_interact(debug_state, input, mouse_pos);
      debug_begin_interact(debug_state, input, mouse_pos);
    }
    
    if (!input->mouse_buttons[Game_input_mouse_button_left].ended_down) {
      debug_end_interact(debug_state, input, mouse_pos);
    }
  }
  else {
    
    debug_state->hot = debug_state->next_hot;
    debug_state->hot_interaction = debug_state->next_hot_interaction;
    
    for (u32 transition_index = input->mouse_buttons[Game_input_mouse_button_left].half_transition_count;
         transition_index > 1; transition_index--) {
      
      debug_begin_interact(debug_state, input, mouse_pos);
      debug_end_interact(debug_state, input, mouse_pos);
    }
    
    if (input->mouse_buttons[Game_input_mouse_button_left].ended_down) {
      debug_begin_interact(debug_state, input, mouse_pos);
    }
  }
  
  debug_state->last_mouse_pos = mouse_pos;
}

static
void
debug_end(Game_input *input, Loaded_bmp *draw_buffer) {
  
  timed_function();
  
  Debug_state *debug_state = debug_get_state();
  
  if (!debug_state)
    return;
  
  Render_group *render_group = debug_state->render_group;
  
  debug_state->next_hot = 0;
  debug_state->next_hot_interaction = Debug_interaction_none;
  Debug_record *hot_record = 0;
  
  v2 mouse_pos = V2(input->mouse_x, input->mouse_y);
  debug_draw_main_menu(debug_state, render_group, mouse_pos);
  debug_interact(debug_state, input, mouse_pos);
  
  if (debug_state->compiling) {
    Debug_process_state state = platform.debug_get_process_state(debug_state->compiler);
    
    if (state.is_running) {
      debug_text_line("compiling");
    }
    else {
      debug_state->compiling = false;
    }
  }
  
  Loaded_font *font = debug_state->debug_font;
  if (!font)
    goto skip_debug_rendering;
  
  Hha_font *font_info = get_font_info(render_group->asset_list, debug_state->font_id);
  
#if 0
  bool render_function_charts = true;
  if (render_function_charts) {  
    for (u32 counter_index = 0; counter_index < debug_state->counter_count; counter_index += 1) {
      Debug_counter_state *counter = debug_state->counter_state_list + counter_index++;
      
      Debug_stats hit_count, cycle_count, cycle_over_hit;
      
      begin_debug_stat(&hit_count);
      begin_debug_stat(&cycle_count);
      begin_debug_stat(&cycle_over_hit);
      
      for (u32 snapshot_index = 0; snapshot_index < DEBUG_SNAPSHOT_COUNT; snapshot_index += 1) {
        accumulate_debug_stats(&hit_count, counter->snapshot_list[snapshot_index].hit_count);
        accumulate_debug_stats(&cycle_count, (u32)counter->snapshot_list[snapshot_index].cycle_count);
        
        f64 cycles_over_hits = .0f;
        
        if (counter->snapshot_list[snapshot_index].hit_count) {
          cycles_over_hits = ((f64)counter->snapshot_list[snapshot_index].cycle_count /
                              (f64)counter->snapshot_list[snapshot_index].hit_count);
        }
        
        accumulate_debug_stats(&cycle_over_hit, cycles_over_hits);
      }
      
      close_debug_stat(&hit_count);
      close_debug_stat(&cycle_count);
      close_debug_stat(&cycle_over_hit);
      
      if (counter->block_name) {
        
        if (hit_count.max > 0.0f) {
          f32 bar_width = 4.0f;
          f32 chart_left = -150.0f;
          f32 chart_min_y = at_y;
          f32 chart_height = font_info->ascender_height * font_scale;
          f32 scale = 1.0f / (f32)cycle_count.max;
          
          for (u32 snapshot_index = 0; snapshot_index < DEBUG_SNAPSHOT_COUNT; snapshot_index += 1) {
            f32 this_proportion = scale * (f32)counter->snapshot_list[snapshot_index].cycle_count;
            f32 this_height = chart_height * this_proportion;
            
            v3 offset = {
              chart_left + bar_width * (f32)snapshot_index + .5f * bar_width,
              chart_min_y +.5f * this_height,
              .0f
            };
            
            v2 size = {bar_width, this_height};
            v4 color = {this_proportion, 1, 0.0, 1};
            
            push_rect(render_group, offset, size, color);
            
          }
        }
        
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer),
                    "%32s(%4d): %10ucy %8uh %10ucy/h", 
                    counter->block_name,
                    counter->line_number,
                    (u32)cycle_count.avg, 
                    (u32)hit_count.avg, 
                    (u32)cycle_over_hit.avg);
        
        debug_text_line(buffer);
      }
      
    }
  }
  
#endif
  
  if (debug_state->frame_count) {
    char text[256];
    _snprintf_s(text, sizeof(text), 
                "last frame time: %.02fms",
                debug_state->frame_list[debug_state->frame_count - 1].wall_seconds_elapsed * 1000.0f);
    
    debug_text_line(text);
  }
  
  if (was_pressed(input->mouse_buttons[Game_input_mouse_button_left])) {
    if (hot_record) 
      debug_state->scope_to_record = hot_record;
    else
      debug_state->scope_to_record = 0;
    
    refresh_collation(debug_state);
  }
  
  
  skip_debug_rendering:
  tiled_render_group_to_output(debug_state->high_priority_queue, debug_state->render_group, draw_buffer);
  end_render(debug_state->render_group);
}

/*
debug_text_line("The quick brown fox jumps over the lazy dog");
debug_text_line("\\#900DEBUG \\#090CYCLE \\#990\\^5COUNTS:");
debug_text_line("AVA Wa Ta");
debug_text_line("\\5C0F\\8033\\6728\\514E");
*/

#define debug_record_list_main_count __COUNTER__
extern u32 debug_record_list_optimized_count;

static Debug_table global_debug_table_;
Debug_table *global_debug_table = &global_debug_table_;

static
Debug_thread*
get_debug_thread(Debug_state *debug_state, u32 thread_id) {
  Debug_thread *result = 0;
  
  for (Debug_thread *thread = debug_state->first_thread; thread; thread = thread->next) {
    
    if (thread->id == thread_id) {
      result = thread;
      break;
    }
    
  }
  
  if (!result) {
    result = mem_push_struct(&debug_state->collate_arena, Debug_thread);
    result->id = thread_id;
    result->lane_index = debug_state->frame_bar_lane_count++;
    result->first_open_block = 0;
    result->next = debug_state->first_thread;
    debug_state->first_thread = result;
  }
  
  return result;
}

static
bool
string_same(char *str_a, char *str_b) {
  
  bool result = false;
  
  if (str_a && str_b) {
    while (*str_a && *str_b && (*str_a == *str_b)) {
      str_a++;
      str_b++;
    }
    
    result = (*str_a == *str_b) && *str_a == 0;
  }
  
  return result;
}


inline 
Debug_record*
get_record_from(Open_debug_block *block) {
  Debug_record *result = block ? block->source : 0;
  
  return result;
}

inline
Debug_frame_region*
add_region(Debug_state *debug_state, Debug_frame *current_frame) {
  assert(current_frame->region_count < MAX_REGIONS_PER_FRAME);
  Debug_frame_region *result = current_frame->region_list + current_frame->region_count++;
  
  return result;
}

static
void
collate_debug_records(Debug_state *debug_state, u32 invalid_event_array_index) {
  
  for (;; debug_state->collation_array_index++) {
    
    if (debug_state->collation_array_index == MAX_DEBUG_EVENT_ARRAY_COUNT) {
      debug_state->collation_array_index = 0;
    }
    
    u32 event_array_index = debug_state->collation_array_index;
    if (event_array_index == invalid_event_array_index)
      break;
    
    u32 event_count = global_debug_table->event_count[event_array_index];
    for (u32 event_index = 0; event_index < event_count; event_index++) {
      Debug_event *event = global_debug_table->event_list[event_array_index] + event_index;
      Debug_record *source = global_debug_table->record_list[event->translation_unit_index] + event->debug_record_index;
      
      if (event->type == Debug_event_type_frame_marker) {
        
        if (debug_state->collation_frame) {
          debug_state->collation_frame->end_clock = event->clock;
          debug_state->collation_frame->wall_seconds_elapsed = event->seconds_elapsed;
          debug_state->frame_count++;
          
          f32 clock_range = (f32)
          (debug_state->collation_frame->end_clock - debug_state->collation_frame->begin_clock);
        }
        
        debug_state->collation_frame = debug_state->frame_list + debug_state->frame_count;
        debug_state->collation_frame->begin_clock = event->clock;
        debug_state->collation_frame->end_clock   = 0;
        debug_state->collation_frame->region_count = 0;
        debug_state->collation_frame->region_list = mem_push_array(&debug_state->collate_arena, MAX_REGIONS_PER_FRAME, Debug_frame_region);
        debug_state->collation_frame->wall_seconds_elapsed = 0.0f;
        
        
      }
      
      else if (debug_state->collation_frame) 
      {
        u32 frame_index = debug_state->frame_count - 1;
        Debug_thread *thread = get_debug_thread(debug_state, event->thread_id__core_id.thread_id);
        u64 relative_clock = event->clock - debug_state->collation_frame->begin_clock;
        
        if (event->type == Debug_event_type_begin_block) {
          
          if (string_same(source->block_name, "tiled_render")) {
            u32 begin = 0;
          }
          
          
          Open_debug_block *debug_block = debug_state->first_free_block;
          
          if (debug_block)
            debug_state->first_free_block = debug_block->next_free;
          else
            debug_block = mem_push_struct(&debug_state->collate_arena, Open_debug_block);
          
          debug_block->starting_frame_index = frame_index;
          debug_block->opening_event = event;
          debug_block->parent = thread->first_open_block;
          debug_block->source = source;
          debug_block->next_free = 0;
          thread->first_open_block = debug_block;
        }
        
        else if (event->type == Debug_event_type_end_block) {
          
          if (string_same(source->block_name, "tiled_render")) {
            u32 end = 0;
          }
          
          if (thread->first_open_block) {
            Open_debug_block *matching_block = thread->first_open_block;
            Debug_event *opening_event = matching_block->opening_event;
            
            if (opening_event->thread_id__core_id.thread_id == event->thread_id__core_id.thread_id &&
                opening_event->debug_record_index == event->debug_record_index &&
                opening_event->translation_unit_index == event->translation_unit_index) {
              
              if (matching_block->starting_frame_index == frame_index) {
                
                if (get_record_from(matching_block->parent) == debug_state->scope_to_record) {
                  f32 min_t = (f32)(opening_event->clock - debug_state->collation_frame->begin_clock);
                  f32 max_t = (f32)(event->clock - debug_state->collation_frame->begin_clock);
                  f32 threshold_t = 0.01f;
                  
                  if (max_t - min_t > threshold_t) {
                    
                    Debug_frame_region *region = add_region(debug_state, debug_state->collation_frame);
                    region->record = source;
                    region->cycle_count = (event->clock - opening_event->clock);
                    region->lane_index = (u16)thread->lane_index;
                    region->min_t = min_t;
                    region->max_t = max_t;
                    region->color_index = (u16)opening_event->debug_record_index;
                  }
                  
                }
                else {
                }
                
                thread->first_open_block->next_free = debug_state->first_free_block;
                debug_state->first_free_block = thread->first_open_block;
                thread->first_open_block = matching_block->parent;
                
              }
              
            }
            else {
            }
            
          }
          
        }
        else {
          assert(!"wrong event type");
        }
        
      }
      
      
    }
    
    
  }
  
}


static 
void
refresh_collation(Debug_state *debug_state) {
  restart_collation(debug_state, global_debug_table->current_event_array_index);
  collate_debug_records(debug_state, global_debug_table->current_event_array_index);
}

extern "C" // to prevent name mangle by compiler, so function can looked up by name exactly
Debug_table*
debug_game_frame_end(Game_memory* memory) {
  
  global_debug_table->record_count[0] = debug_record_list_main_count;
  global_debug_table->record_count[1] = debug_record_list_optimized_count;
  
  ++global_debug_table->current_event_array_index;
  if (global_debug_table->current_event_array_index >= array_count(global_debug_table->event_list)) {
    global_debug_table->current_event_array_index = 0;
  }
  
  u64 array_index__event_index = atomic_exchange_u64(&global_debug_table->event_array_index__event_index, (u64)global_debug_table->current_event_array_index << 32);
  
  u32 event_array_index = array_index__event_index >> 32;
  u32 event_count = array_index__event_index & 0xffffffff;
  global_debug_table->event_count[event_array_index] = event_count;
  
  Debug_state *debug_state = debug_get_state(memory);
  if (debug_state) {
    
    if (memory->executable_reloaded) {
      restart_collation(debug_state, global_debug_table->current_event_array_index);
    }
    
    if (!debug_state->paused) {
      
      if (debug_state->frame_count >= MAX_DEBUG_EVENT_ARRAY_COUNT * 4) {
        restart_collation(debug_state, global_debug_table->current_event_array_index);
      }
      collate_debug_records(debug_state, global_debug_table->current_event_array_index);
      
    }
  }
  
  return global_debug_table;
}
