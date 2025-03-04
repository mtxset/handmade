static f32 left_edge;
static f32 at_y;
static f32 font_scale;
static Font_id font_id;

static
void
debug_reset(Game_asset_list *asset_list, u32 width, u32 height) {
  timed_block();
  Asset_vector match_vector = {};
  Asset_vector weight_vector = {};
  match_vector.e[Tag_font_type] = (f32)Font_type_debug;
  weight_vector.e[Tag_font_type] = 1.0f;
  
  font_id = get_best_match_font_from(asset_list, Asset_font, &match_vector, &weight_vector);
  
  font_scale = 1.0f;
  ortographic(debug_render_group, width, height, 1.0f);
  left_edge = -.5f * width;
  
  Hha_font *info = get_font_info(asset_list, font_id);
  at_y = 0.5f * height - font_scale * get_starting_baseline_y(info);
}

static
void
update_debug_records(Debug_state *debug_state, u32 counter_count, Debug_record *record_list) {
  
  for (u32 counter_index = 0; counter_index < counter_count; counter_index += 1) {
    Debug_record        *src = record_list + counter_index;
    Debug_counter_state *dst = debug_state->counter_state_list + debug_state->counter_count++;
    
    if (false) {
      u32 k = 0;
    }
    
    u64 hit_count_cycle_count = atomic_exchange_u64(&src->hit_count_cycle_count, 0);
    u32 hit_count = (u32)(hit_count_cycle_count >> 32);
    u32 cycle_count = (u32)(hit_count_cycle_count & 0xffffffff);
    
    dst->file_name = src->file_name;
    dst->function_name = src->function_name;
    dst->line_number = src->line_number;
    dst->snapshot_list[debug_state->snapshot_index].hit_count = hit_count;
    dst->snapshot_list[debug_state->snapshot_index].cycle_count = cycle_count;
  }
}

static
void
debug_text_line(char *string) {
  
  if (!debug_render_group)
    return;
  
  Render_group *render_group = debug_render_group;
  
  Loaded_font *font = push_font(render_group, font_id);
  
  if (!font)
    return;
  
  Hha_font *info = get_font_info(render_group->asset_list, font_id);
  
  u32 prev_code_point = 0;
  f32 char_scale = font_scale;
  v4 color = white_v4;
  f32 at_x = left_edge;
  
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
      char_scale = font_scale * clamp01(scale * (f32)(at[2] - '0'));
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
        
        Bitmap_id id = get_bitmap_for_glyph(render_group->asset_list, info, font, code_point);
        Hha_bitmap *bitmap_info = get_bitmap_info(render_group->asset_list, id);
        
        f32 size = (f32)bitmap_info->dim[1];
        
        push_bitmap(render_group, id, char_scale * size, v3{at_x, at_y, 0}, color);
      }
      
      prev_code_point = code_point;
      at++;
    }
  }
  
  at_y -= get_line_advance_for(info) * font_scale;
}

static
void
debug_overlay(Game_memory *memory) {
  
#if 0
  debug_text_line("The quick brown fox jumps over the lazy dog");
  debug_text_line("\\#900DEBUG \\#090CYCLE \\#990\\^5COUNTS:");
  debug_text_line("AVA Wa Ta");
  debug_text_line("\\5C0F\\8033\\6728\\514E");
#endif
  
  Debug_state *debug_state = (Debug_state*)memory->debug_storage;
  
  if (!debug_state || !debug_render_group)
    return;
  
  Render_group *render_group = debug_render_group;
  
  Loaded_font *font = push_font(render_group, font_id);
  if (!font)
    return;
  
  Hha_font *font_info = get_font_info(render_group->asset_list, font_id);
  
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
        accumulate_debug_stats(&cycle_count, counter->snapshot_list[snapshot_index].cycle_count);
        
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
      
      if (counter->function_name) {
        
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
                    counter->function_name,
                    counter->line_number,
                    (u32)cycle_count.avg, 
                    (u32)hit_count.avg, 
                    (u32)cycle_over_hit.avg);
        
        debug_text_line(buffer);
      }
      
    }
  }
  
  f32 bar_width    = 8.0f;
  f32 bar_spacing  = 10.0f;
  f32 chart_left   = left_edge + 10.0f;
  f32 chart_height = 200.0f;
  f32 chart_width  = bar_spacing * (f32)DEBUG_SNAPSHOT_COUNT;
  f32 chart_min_y  = at_y - (chart_height + 250.0f);
  f32 scale = 1.0f / 0.03333f;
  
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
  
  for (u32 snapshot_index = 0; snapshot_index < DEBUG_SNAPSHOT_COUNT; snapshot_index += 1) {
    Debug_frame_end_info *info = debug_state->frame_end_info_list + snapshot_index;
    
    f32 stack_y = chart_min_y;
    f32 prev_ts_seconds = 0.0f;
    
    for (u32 ts_index = 0; ts_index < info->timestamp_count; ts_index += 1) {
      Debug_frame_timestamp *timestamp = info->timestamp_list + ts_index;
      
      f32 seconds_elapsed = timestamp->seconds - prev_ts_seconds;
      prev_ts_seconds = timestamp->seconds;
      
      v3 color = color_list[ts_index % array_count(color_list)];
      f32 this_proportion = scale * seconds_elapsed;
      f32 this_height = chart_height * this_proportion;
      
      v3 offset = {
        chart_left + bar_spacing * (f32)snapshot_index + .5f * bar_width,
        stack_y +.5f * this_height,
        .0f
      };
      
      v2 size = {bar_width, this_height};
      v4 final_color = {color.r, color.g, color.b, 1.0f};
      
      push_rect(render_group, offset, size, final_color);
      stack_y += this_height;
    }
    
    v3 offset = {
      chart_left + .5f * (f32)chart_width,
      chart_min_y + chart_height,
      .0f
    };
    
    v2 size = {chart_width, 1.0f};
    
    push_rect(render_group, offset, size, white_v4);
  }
  
}

Debug_record debug_record_list[__COUNTER__];

extern u32 const optimized_debug_record_list_count;
Debug_record optimized_debug_record_list[];

u32 const main_debug_record_list_count = array_count(debug_record_list);
//Debug_record main_debug_record_list[main_debug_record_list_count]; defined in build.bat

extern "C" // to prevent name mangle by compiler, so function can looked up by name exactly
void 
debug_game_frame_end(Game_memory* memory, Debug_frame_end_info *info) {
  
  Debug_state *debug_state = (Debug_state*)memory->debug_storage;
  
  if (debug_state) {
    debug_state->counter_count = 0;
    
    update_debug_records(debug_state, optimized_debug_record_list_count, optimized_debug_record_list);
    update_debug_records(debug_state, main_debug_record_list_count, main_debug_record_list);
    
    debug_state->frame_end_info_list[debug_state->snapshot_index] = *info;
    
    debug_state->snapshot_index++;
    
    if (debug_state->snapshot_index >= DEBUG_SNAPSHOT_COUNT) {
      debug_state->snapshot_index = 0;
    }
    
  }
  
}
