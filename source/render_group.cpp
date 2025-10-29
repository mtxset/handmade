#include "color.h"

inline
v4
unpack_4x8(u32 packed) {
  v4 result; 
  
  result = {
    (f32)((packed >> 16) & 0xff),
    (f32)((packed >> 8)  & 0xff),
    (f32)((packed >> 0)  & 0xff),
    (f32)((packed >> 24) & 0xff)
  };
  
  return result;
}

inline
v4
unscale_bias_normal(v4 normal) {
  v4 result;
  
  f32 inv_255 = 1.0f / 255.0f;
  
  result.x = -1.0f + 2.0f * (inv_255 * normal.x);
  result.y = -1.0f + 2.0f * (inv_255 * normal.y);
  result.z = -1.0f + 2.0f * (inv_255 * normal.z);
  
  result.w = inv_255 * normal.w;
  
  return result;
}

inline
Entity_basis_result
get_render_entity_basis_pos(Render_transform *transform, v3 original_pos) {
  
  timed_function();
  
  Entity_basis_result result = {};
  
  v3 pos = v2_to_v3(original_pos.xy, 0) + transform->offset_pos;
  
  if (transform->orthographic) {
    result.pos = transform->screen_center + transform->meters_to_pixels * pos.xy;
    result.scale = transform->meters_to_pixels;
    result.valid = true;
  }
  else {
    
    f32 offzet_z = 0.0f;
    
    f32 dist_above_target = transform->dist_above_target;
    
#if DEBUG_UI_use_debug_camera
    bool debug_camera = true;
    if (debug_camera) {
      dist_above_target += 50.0f;
    }
#endif
    
    f32 dist_to_p = dist_above_target - pos.z;
    f32 near_clip_plane = 0.2f;
    
    v3 raw_xy = v2_to_v3(pos.xy, 1.0f);
    
    if (dist_to_p > near_clip_plane) {
      v3 projected_xy = (1.0f / dist_to_p) * transform->focal_length * raw_xy;
      
      result.scale = transform->meters_to_pixels * projected_xy.z;
      result.pos = transform->screen_center + transform->meters_to_pixels * projected_xy.xy +
        v2 {0, result.scale * offzet_z};
      result.valid = true;
    }
  }
  
  return result;
}


#define push_render_element(group,type) (type*)push_render_element_(group, sizeof(type), Render_group_entry_type_##type)

inline
void*
push_render_element_(Render_group* group, u32 size, Render_group_entry_type type) {
  
  timed_function();
  
  void* result = 0;
  
  size += sizeof(Render_group_entry_header);
  
  if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
    auto header = (Render_group_entry_header*)(group->push_buffer_base + group->push_buffer_size);
    header->type = type;
    result = (u8*)header + sizeof(*header);
    group->push_buffer_size += size;
  }
  else {
    assert(!"INVALID");
  }
  
  return result;
}

inline
Used_bitmap_dim
get_bitmap_dim(Render_group *group, Loaded_bmp *bitmap, f32 height, v3 offset) {
  Used_bitmap_dim dim;
  
  dim.size  = V2(height * bitmap->width_over_height, height);
  dim.align = hadamard(bitmap->align_pcent, dim.size);
  dim.pos   = offset - V3(dim.align, 0);
  dim.basis = get_render_entity_basis_pos(&group->transform, dim.pos);
  
  return dim;
}

inline
void
push_bitmap(Render_group* group, Loaded_bmp* bitmap, f32 height, v3 offset, v4 color = white_v4) {
  
  timed_function();
  
  Used_bitmap_dim dim = get_bitmap_dim(group, bitmap, height, offset);
  
  if (!dim.basis.valid)
    return;
  
  Render_entry_bitmap* entry = push_render_element(group, Render_entry_bitmap);
  
  if (!entry)
    return;
  
  entry->bitmap = bitmap;
  entry->pos = dim.basis.pos;
  entry->color = group->global_alpha * color;
  entry->size = dim.basis.scale * dim.size;
}

inline
void
push_bitmap(Render_group* group, Bitmap_id id, f32 height, v3 offset, v4 color = white_v4) {
  
  timed_function();
  
  Loaded_bmp *bitmap = get_bitmap(group->asset_list, id, group->generation_id);
  
  if (group->renders_in_background && !bitmap) {
    load_bitmap(group->asset_list, id, _(immediate)true);
    bitmap = get_bitmap(group->asset_list, id, group->generation_id);
  }
  
  if (bitmap) {
    push_bitmap(group, bitmap, height, offset, color);
  }
  else {
    assert(!group->renders_in_background);
    load_bitmap(group->asset_list, id, _(immediate)false);
    group->missing_resource_count++;
  }
  
}

inline
void
push_rect(Render_group* group, v3 offset, v2 dim, v4 color = white_v4) {
  
  timed_function();
  v3 pos = offset - v2_to_v3(0.5 * dim, 0);
  Entity_basis_result basis = get_render_entity_basis_pos(&group->transform, pos);
  
  if (!basis.valid)
    return;
  
  Render_entry_rect* rect = push_render_element(group, Render_entry_rect);
  
  if (!rect)
    return;
  
  rect->pos   = basis.pos;
  rect->color = color;
  rect->dim   = basis.scale * dim;
}

inline
void
push_rect(Render_group *group, Rect2 rect, f32 z, v4 color = white_v4) {
  push_rect(group, V3(get_center(rect), z), get_dim(rect), color);
}

inline
void
push_rect_outline(Render_group* group, v3 offset, v2 dim, v4 color = white_v4) {
  
  f32 thickness = 0.1f;
  push_rect(group, offset - v3{0, 0.5f * dim.y, 0}, v2{dim.x, thickness}, color);
  push_rect(group, offset + v3{0, 0.5f * dim.y, 0} ,v2{dim.x, thickness}, color);
  
  push_rect(group, offset - v3{0.5f * dim.x, 0, 0}, v2{thickness, dim.y}, color);
  push_rect(group, offset + v3{0.5f * dim.x, 0, 0}, v2{thickness, dim.y}, color);
}

internal
void
draw_rect_old(Loaded_bmp* buffer, v2 start, v2 end, v4 color) {
  timed_function();
  u32 color_hex = 0;
  
  color_hex = (round_f32_u32(color.a * 255.0f) << 24 | 
               round_f32_u32(color.r * 255.0f) << 16 | 
               round_f32_u32(color.g * 255.0f) << 8  |
               round_f32_u32(color.b * 255.0f) << 0);
  
  u32 x0 = round_f32_u32(start.x); u32 x1 = round_f32_u32(end.x);
  u32 y0 = round_f32_u32(start.y); u32 y1 = round_f32_u32(end.y);
  
  x0 = clamp_i32(x0, 0, buffer->width);  x1 = clamp_i32(x1, 0, buffer->width);
  y0 = clamp_i32(y0, 0, buffer->height); y1 = clamp_i32(y1, 0, buffer->height);
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8* row = (u8*)buffer->memory + x0 * bytes_per_pixel + y0 * buffer->pitch;
  
  for (u32 y = y0; y < y1; y++) {
    u32* pixel = (u32*)row;
    for (u32 x = x0; x < x1; x++) {
      *pixel++ = color_hex;
    }
    row += buffer->pitch;
  }
}

internal
void
draw_rect(Loaded_bmp* buffer, v2 v_min, v2 v_max, v4 color, Rect2i clip_rect, bool even) {
  
  timed_function();
  
  u32 color_hex = (round_f32_u32(color.a * 255.0f) << 24 | 
                   round_f32_u32(color.r * 255.0f) << 16 | 
                   round_f32_u32(color.g * 255.0f) << 8  |
                   round_f32_u32(color.b * 255.0f) << 0);
  
  Rect2i fill_rect;
  fill_rect.min_x = round_f32_i32(v_min.x);
  fill_rect.min_y = round_f32_i32(v_min.y);
  fill_rect.max_x = round_f32_i32(v_max.x);
  fill_rect.max_y = round_f32_i32(v_max.y);
  
  fill_rect = rect_intersect(fill_rect, clip_rect);
  
  if (!even == (fill_rect.min_y & 1)) {
    fill_rect.min_y += 1;
  }
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8* row = ((u8*)buffer->memory + 
             fill_rect.min_x * bytes_per_pixel + 
             fill_rect.min_y * buffer->pitch);
  
  for (i32 y = fill_rect.min_y; y < fill_rect.max_y; y += 2) {
    u32* pixel = (u32*)row;
    for (i32 x = fill_rect.min_x; x < fill_rect.max_x; x++) {
      *pixel++ = color_hex;
    }
    row += 2 * buffer->pitch;
  }
}

struct Bilinear_sample {
  u32 a, b, c, d;
};

inline
Bilinear_sample
bilinear_sample(Loaded_bmp* bitmap, i32 x, i32 y) {
  Bilinear_sample result;
  
  u8* texel_ptr = ((u8*)bitmap->memory) + y * bitmap->pitch + x * sizeof(u32);
  result.a = *(u32*) texel_ptr;
  result.b = *(u32*)(texel_ptr + sizeof(u32));
  result.c = *(u32*)(texel_ptr + bitmap->pitch);
  result.d = *(u32*)(texel_ptr + bitmap->pitch + sizeof(u32));
  
  return result;
}

inline 
v4
srgb_bilinear_blend(Bilinear_sample texel_sample, f32 f_x, f32 f_y) {
  v4 texel_a = unpack_4x8(texel_sample.a);
  v4 texel_b = unpack_4x8(texel_sample.b);
  v4 texel_c = unpack_4x8(texel_sample.c);
  v4 texel_d = unpack_4x8(texel_sample.d);
  
  texel_a = srgb255_to_linear1(texel_a);
  texel_b = srgb255_to_linear1(texel_b);
  texel_c = srgb255_to_linear1(texel_c);
  texel_d = srgb255_to_linear1(texel_d);
  
  v4 result = lerp(lerp(texel_a, f_x, texel_b),
                   f_y,
                   lerp(texel_c, f_x, texel_d));
  
  return result;
}

inline
v3
sample_env_map(v2 screen_space_uv, v3 sample_direction, f32 roughness, Env_map* map, f32 distance_from_map_z) {
  
  /*
screen_space_uv - says where ray is being casted from in normalized screen coords.

sample_direction - what direction cast is goind - not normalized

roughness - which LODs of map we sample from 

distance_from_map_z - how far the map is from sample point in z, in meters
*/
  
  // pick LOD
  u32 lod_index = (u32)(roughness * (f32)(array_count(map->lod) - 1) + 0.05f);
  assert(lod_index < array_count(map->lod));
  
  Loaded_bmp* lod = &map->lod[lod_index];
  
  // compute distance to the map and the scaling factor for meters-to-uv
  f32 uv_per_meter = 0.1f;
  f32 c = (uv_per_meter * distance_from_map_z) / sample_direction.y;
  v2 offset = c * v2{sample_direction.x, sample_direction.z};
  
  // find the intersection point
  v2 uv = screen_space_uv + offset;
  
  // clamp it to valid range
  uv.x = clamp01(uv.x);
  uv.y = clamp01(uv.y);
  
  // bilinear sample 
  f32 t_x = uv.x * (f32)(lod->width  - 2);
  f32 t_y = uv.y * (f32)(lod->height - 2);
  
  i32 x = (i32)t_x;
  i32 y = (i32)t_y;
  
  f32 f_x = t_x -(f32)x;
  f32 f_y = t_y -(f32)y;
  
  assert(x >= 0 && x < lod->width);
  assert(y >= 0 && y < lod->height);
  
  bool show_where_sampling_is_coming_from = false;
  
  if (show_where_sampling_is_coming_from) {
    u8* texel_ptr = ((u8*)lod->memory) + y * lod->pitch + x * sizeof(u32);
    *(u32*)texel_ptr = 0xffffffff;
  }
  
  Bilinear_sample sample = bilinear_sample(lod, x, y);
  v3 result = srgb_bilinear_blend(sample, f_x, f_y).xyz;
  
  return result;
}

internal
void
draw_rect_slow(Loaded_bmp* buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap, Loaded_bmp* normal_map, Env_map* top, Env_map* middle, Env_map* bottom, f32 pixel_to_meter) {
  timed_function();
  
  color.rgb *= color.a;
  
  f32 inverse_x_axis_squared = 1.0f / length_squared(x_axis);
  f32 inverse_y_axis_squared = 1.0f / length_squared(y_axis);
  
  u32 color_hex = (round_f32_u32(color.a * 255.0f) << 24 | 
                   round_f32_u32(color.r * 255.0f) << 16 | 
                   round_f32_u32(color.g * 255.0f) << 8  |
                   round_f32_u32(color.b * 255.0f) << 0);
  
  i32 width_max  = buffer->width - 1;
  i32 height_max = buffer->height - 1;
  
  f32 inv_width_max  = 1.0f / (f32)width_max;
  f32 inv_height_max = 1.0f / (f32)height_max;
  
  i32 x_min = width_max;
  i32 x_max = 0;
  i32 y_min = height_max;
  i32 y_max = 0;
  
  v2 p[4] = {
    origin,
    origin + x_axis,
    origin + x_axis + y_axis, 
    origin + y_axis
  };
  
  for (i32 i = 0; i < array_count(p); i++) {
    v2 test_pos = p[i];
    
    int floor_x = floor_f32_i32(test_pos.x);
    int ceil_x  = ceil_f32_i32(test_pos.x);
    int floor_y = floor_f32_i32(test_pos.y);
    int ceil_y  = ceil_f32_i32(test_pos.y);
    
    if (x_min > floor_x) x_min = floor_x;
    if (y_min > floor_y) y_min = floor_y;
    if (x_max < ceil_x) x_max = ceil_x;
    if (y_max < ceil_y) y_max = ceil_y;
  }
  
  if (x_min < 0) x_min = 0;
  if (y_min < 0) y_min = 0;
  if (x_max > width_max) x_max = width_max;
  if (y_max > height_max) y_max = height_max;
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8* row = (u8*)buffer->memory + x_min * bytes_per_pixel + y_min * buffer->pitch;
  
  f32 origin_z = 0.0;
  f32 origin_y = (origin + 0.5f * x_axis + 0.5f * y_axis).y;
  f32 fixed_cast_y = inv_height_max * origin_y;
  
  u64 pixel_cycle_timer = __rdtsc();
  for (i32 y = y_min; y <= y_max; y++) {
    u32* pixel = (u32*)row;
    for (i32 x = x_min; x <= x_max; x++) {
      
      v2 pixel_pos = v2_i32(x, y);
      v2 d = pixel_pos - origin;
      
      f32 edge0 = inner(d, -perpendicular(x_axis));
      f32 edge1 = inner(d - x_axis, -perpendicular(y_axis));
      f32 edge2 = inner(d - x_axis - y_axis, perpendicular(x_axis));
      f32 edge3 = inner(d - y_axis, perpendicular(y_axis));
      
      if (edge0 < 0 && 
          edge1 < 0 &&
          edge2 < 0 && 
          edge3 < 0) {
        
        v2 screen_space_uv = {
          inv_width_max  * (f32)x, 
          fixed_cast_y
        };
        
        f32 z_diff = pixel_to_meter * ((f32)y - origin_y);
        
        f32 u = inverse_x_axis_squared * inner(d, x_axis);
        f32 v = inverse_y_axis_squared * inner(d, y_axis);
        
        //assert(u >= 0.0f && u <= 1.0f);
        //assert(v >= 0.0f && v <= 1.0f);
        
        f32 t_x = (u * ((f32)(bitmap->width - 2)));
        f32 t_y = (v * ((f32)(bitmap->height - 2)));
        
        i32 x_i32 = (i32)t_x;
        i32 y_i32 = (i32)t_y;
        
        f32 f_x = t_x - (f32)x_i32;
        f32 f_y = t_y - (f32)y_i32;
        
        assert(x_i32 >= 0 && x_i32 < bitmap->width);
        assert(y_i32 >= 0 && y_i32 < bitmap->height);
        
        Bilinear_sample texel_sample = bilinear_sample(bitmap, x_i32, y_i32);
        v4 texel = srgb_bilinear_blend(texel_sample, f_x, f_y);
        
        // normal maps and lighting
        if (normal_map) {
          
          Bilinear_sample normal_sample = bilinear_sample(normal_map, x_i32, y_i32);
          
          v4 normal_a = unpack_4x8(normal_sample.a);
          v4 normal_b = unpack_4x8(normal_sample.b);
          v4 normal_c = unpack_4x8(normal_sample.c);
          v4 normal_d = unpack_4x8(normal_sample.d);
          
          v4 normal = 
            lerp(lerp(normal_a, f_x, normal_b),
                 f_y,
                 lerp(normal_c, f_x, normal_d));
          
          normal = unscale_bias_normal(normal);
          
          // rotate normals
          // day 102
          {
            f32 x_axis_length = length(x_axis);
            f32 y_axis_length = length(y_axis);
            
            v2 normal_x_axis = (y_axis_length / x_axis_length) * x_axis;
            v2 normal_y_axis = (x_axis_length / y_axis_length) * y_axis;
            
            f32 z_scale = 0.5f * (x_axis_length + y_axis_length);
            normal.xy = normal.x * normal_x_axis + normal.y * normal_y_axis;
            normal.z *= z_scale;
          }
          
          normal.xyz = normalize(normal.xyz);
          
          // eye vectors is always -> v3 eye_vector = {0,0,1} = e;
          // simplified version of reflection -e + 2e^T N N 
          v3 bounce_direction = 2.0f * normal.z * normal.xyz;
          bounce_direction.z -= 1.0f;
          
          bounce_direction.z = -bounce_direction.z;
          
          Env_map* far_map = 0;
          f32 pos_z = origin_z + z_diff;
          f32 map_z = 2.0f;
          f32 t_env_map = bounce_direction.y;
          f32 t_far_map = 0.0f;
          
          if (t_env_map < -0.5f) {
            far_map = bottom;
            t_far_map = -1.0f - 2.0f * t_env_map;
          }
          else if (t_env_map > 0.5) {
            far_map = top;
            t_far_map = 2.0f * (t_env_map - 0.5f);
          }
          
          t_far_map *= t_far_map;
          t_far_map *= t_far_map;
          
          v3 light_color = {0,0,0}; // sample_env_map(screen_space_uv, normal.xyz, normal.w, middle);
          
          if (far_map) {
            f32 distance_from_map_z = far_map->pos_z - pos_z;
            
            v3 far_map_color = sample_env_map(screen_space_uv, bounce_direction, normal.w, far_map, distance_from_map_z);
            light_color = lerp(light_color, t_far_map, far_map_color);
          }
          texel.rgb = texel.rgb + texel.a * light_color;
          
          bool draw_bounce_direction = false;
          
          if (draw_bounce_direction) {
            texel.rgb = v3{0.5f, 0.5f, 0.5f} + 0.5f * bounce_direction;
            texel.rgb *= texel.a;
          }
        }
        
        // color blending
        {
          texel = hadamard(texel, color);
          
          texel.r = clamp01(texel.r);
          texel.g = clamp01(texel.g);
          texel.b = clamp01(texel.b);
          
          v4 dst = {
            (f32)((*pixel >> 16) & 0xff),
            (f32)((*pixel >> 8)  & 0xff),
            (f32)((*pixel >> 0)  & 0xff),
            (f32)((*pixel >> 24) & 0xff),
          };
          
          // converting back
          dst = srgb255_to_linear1(dst);
          
          // (1-t)A + tB 
          // alpha (0 - 1);
          // color = dst_color + alpha (src_color - dst_color)
          // color = (1 - alpha)dst_color + alpha * src_color;
          
          // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
          
          // linear blending
          v4 blended = (1.0f - texel.a) * dst + texel;
          
          v4 blended_255 = linear1_to_srgb255(blended);
          
          *pixel = (((u32)(blended_255.a + 0.5f) << 24)|
                    ((u32)(blended_255.r + 0.5f) << 16)|
                    ((u32)(blended_255.g + 0.5f) << 8 )|
                    ((u32)(blended_255.b + 0.5f) << 0 ));
          
        }
        
        //*pixel = texel;
      }
      pixel++;
    }
    row += buffer->pitch;
  }
  
}

internal
void
change_saturation(Loaded_bmp* bitmap_buffer, f32 level) {
  
  timed_function();
  
  u8* dest_row = (u8*)bitmap_buffer->memory;
  
  for (i32 y = 0; y < bitmap_buffer->height; y++) {
    u32* dest = (u32*)dest_row; // incoming pixel
    
    for (i32 x = 0; x < bitmap_buffer->width; x++) {
      
      v4 dst = {
        (f32)((*dest >> 16) & 0xff),
        (f32)((*dest >> 8)  & 0xff),
        (f32)((*dest >> 0)  & 0xff),
        (f32)((*dest >> 24) & 0xff),
      };
      
      dst = srgb255_to_linear1(dst);
      
      f32 avg = (1.0f / 3.0f) * (dst.r + dst.g + dst.b);
      v3 delta = {
        dst.r - avg,
        dst.g - avg,
        dst.b - avg
      };
      
      v4 result = to_v4(v3{avg, avg, avg} + level * delta, dst.a);
      result = linear1_to_srgb255(result);
      
      *dest = (((u32)(result.a + 0.5f) << 24)|
               ((u32)(result.r + 0.5f) << 16)|
               ((u32)(result.g + 0.5f) << 8 )|
               ((u32)(result.b + 0.5f) << 0 ));
      
      dest++;
    }
    
    dest_row += bitmap_buffer->pitch;
  }
}

internal
void
draw_bitmap(Loaded_bmp* bitmap_buffer, Loaded_bmp* bitmap, v2 start, f32 c_alpha = 1.0f) {
  
  timed_function();
  
  i32 min_x = round_f32_i32(start.x);
  i32 min_y = round_f32_i32(start.y);
  i32 max_x = min_x + bitmap->width;
  i32 max_y = min_y + bitmap->height;
  
  i32 source_offset_x = 0;
  i32 source_offset_y = 0;
  
  if (min_x < 0) {
    source_offset_x = -min_x;
    min_x = 0;
  }
  
  if (min_y < 0) {
    source_offset_y = -min_y;
    min_y = 0;
  }
  
  if (max_x > bitmap_buffer->width) {
    max_x = bitmap_buffer->width;
  }
  
  if (max_y > bitmap_buffer->height) {
    max_y = bitmap_buffer->height;
  }
  
  c_alpha = clamp_f32(c_alpha, 0.0f, 1.0f);
  // reading bottom up
  //* (bitmap->height - 1) 
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8* source_row = ((u8*)bitmap->memory + source_offset_y * bitmap->pitch + bytes_per_pixel * source_offset_x);
  assert(source_row);
  u8* dest_row = ((u8*)bitmap_buffer->memory +
                  min_x * bytes_per_pixel   + 
                  min_y * bitmap_buffer->pitch);
  
  for (i32 y = min_y; y < max_y; y++) {
    u32* dest = (u32*)dest_row; // incoming pixel
    u32* source = (u32*)source_row;   // backbuffer pixel
    
    for (i32 x = min_x; x < max_x; x++) {
      
      v4 texel = {
        (f32)((*source >> 16) & 0xff),
        (f32)((*source >> 8)  & 0xff),
        (f32)((*source >> 0)  & 0xff),
        (f32)((*source >> 24) & 0xff),
      };
      
      texel = srgb255_to_linear1(texel);
      texel *= c_alpha;
      
      
      v4 dst = {
        (f32)((*dest >> 16) & 0xff),
        (f32)((*dest >> 8)  & 0xff),
        (f32)((*dest >> 0)  & 0xff),
        (f32)((*dest >> 24) & 0xff),
      };
      
      dst = srgb255_to_linear1(dst);
      
      // (1-t)A + tB 
      // alpha (0 - 1);
      // color = dst_color + alpha (src_color - dst_color)
      // color = (1 - alpha)dst_color + alpha * src_color;
      
      // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
      
      // linear blending
      f32 inv_r_src_alpha = 1.0f - texel.a;
      
      v4 result = {
        inv_r_src_alpha * dst.r + texel.r,
        inv_r_src_alpha * dst.g + texel.g,
        inv_r_src_alpha * dst.b + texel.b,
        texel.a + dst.a - texel.a * dst.a
      };
      
      result = linear1_to_srgb255(result);
      
      *dest = (((u32)(result.a + 0.5f) << 24)|
               ((u32)(result.r + 0.5f) << 16)|
               ((u32)(result.g + 0.5f) << 8 )|
               ((u32)(result.b + 0.5f) << 0 ));
      
      dest++;
      source++;
    }
    
    dest_row += bitmap_buffer->pitch;
    source_row += bitmap->pitch;
  }
}

internal
void
render_group_to_output(Render_group* render_group, Loaded_bmp* output_target, Rect2i clip_rect, bool even) {
  
  timed_function();
  
  f32 null_pixels_to_meters = 1.0f;
  
  for (u32 base_addr = 0; base_addr < render_group->push_buffer_size;) {
    
    auto header = (Render_group_entry_header*)(render_group->push_buffer_base + base_addr);
    base_addr += sizeof(*header);
    
    void* data = (u8*)header + sizeof(*header);
    switch (header->type) {
      
      case Render_group_entry_type_Render_entry_clear: {
        Render_entry_clear* entry = (Render_entry_clear*)data;
        base_addr += sizeof(*entry);
        
        v2 end = {
          (f32)output_target->width,
          (f32)output_target->height
        };
        draw_rect(output_target, v2{0,0}, end, entry->color, clip_rect, even);
        
      } break;
      
      case Render_group_entry_type_Render_entry_saturation: {
        Render_entry_saturation* entry = (Render_entry_saturation*)data;
        base_addr += sizeof(*entry);
        
        change_saturation(output_target, entry->level);
        
      } break;
      
      case Render_group_entry_type_Render_entry_bitmap: {
        Render_entry_bitmap* entry = (Render_entry_bitmap*)data;
        
        assert(entry->bitmap);
        
#if 0
        draw_rect_slow(output_target, entry->pos, 
                       { entry->size.x, 0 },
                       { 0, entry->size.y },
                       entry->color,
                       entry->bitmap, 0,0,0,0, null_pixels_to_meters);
#else
        draw_rect_quak(output_target, entry->pos, 
                       { entry->size.x, 0 },
                       { 0, entry->size.y },
                       entry->color,
                       entry->bitmap, null_pixels_to_meters, 
                       clip_rect, even);
#endif
        
        base_addr += sizeof(*entry);
      } break;
      
      case Render_group_entry_type_Render_entry_rect: {
        Render_entry_rect* entry = (Render_entry_rect*)data;
        base_addr += sizeof(*entry);
        
        draw_rect(output_target, entry->pos, entry->pos + entry->dim, entry->color, clip_rect, even);
      } break;
      
      case Render_group_entry_type_Render_entry_coord_system: {
        Render_entry_coord_system* entry = (Render_entry_coord_system*)data;
        base_addr += sizeof(*entry);
        
        v2 pos = entry->origin;
        v2 dim = v2{2,2};
        
        v2 end = entry->origin + entry->x_axis + entry->y_axis;
        draw_rect_slow(output_target, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->bitmap, entry->normal_map, entry->top, entry->middle, entry->bottom, null_pixels_to_meters);
        
        v4 corner_color = yellow_v4;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        pos = entry->origin + entry->x_axis;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        pos = entry->origin + entry->y_axis;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        draw_rect(output_target, end - dim, end + dim, corner_color, clip_rect, even);
#if 0
        for (u32 i = 0; i < array_count(entry->points); i++) {
          v2 p = entry->points[i];
          p = entry->origin + p.x * entry->x_axis + p.y * entry->y_axis;
          draw_rect(output_target, p - dim, p + dim, corner_color);
        }
#endif
      } break;
      
      default: {
        assert(!"CRASH");
      } break;
    }
  }
}

struct Tile_render_work
{
  Render_group *render_group;
  Loaded_bmp *output_target;
  Rect2i clip_rect;
};

internal
PLATFORM_WORK_QUEUE_CALLBACK(do_tile_render_work)
{
  Tile_render_work *work = (Tile_render_work*)data;
  
  bool even = true;
  bool not_even = false;
  
  render_group_to_output(work->render_group, work->output_target, work->clip_rect, even);
  render_group_to_output(work->render_group, work->output_target, work->clip_rect, not_even);
}

internal
void
render_group_to_output(Render_group *render_group, Loaded_bmp *output_target) {
  
  timed_function();
  
  assert(((uintptr)output_target->memory & 15) == 0);
  
  Rect2i clip_rect;
  
  clip_rect.min_x = 0;
  clip_rect.min_y = 0;
  
  clip_rect.max_x = output_target->width;
  clip_rect.max_y = output_target->height;
  
  Tile_render_work work;
  work.render_group = render_group;
  work.output_target = output_target;
  work.clip_rect = clip_rect;
  
  do_tile_render_work(0, &work);
}


internal
void
tiled_render_group_to_output(Platform_work_queue *render_queue, 
                             Render_group *render_group, Loaded_bmp *output_target) {
  
  timed_function();
  
  i32 const tile_count_x = 9;
  i32 const tile_count_y = 9;
  Tile_render_work work_array[tile_count_y * tile_count_x];
  
  assert(((uintptr)output_target->memory & 15) == 0);
  
  i32 tile_width = output_target->width / tile_count_x;
  i32 tile_height = output_target->height / tile_count_y;
  
  tile_width = ((tile_width + 3) / 4) * 4;
  
  i32 work_count = 0;
  for (i32 tile_y = 0; tile_y < tile_count_y; tile_y++) {
    for (i32 tile_x = 0; tile_x < tile_count_x; tile_x++) {
      
      Tile_render_work *work = work_array + work_count++;
      
      Rect2i clip_rect;
      
      clip_rect.min_x = tile_x * tile_width;
      clip_rect.min_y = tile_y * tile_height;
      
      clip_rect.max_x = clip_rect.min_x + tile_width;
      clip_rect.max_y = clip_rect.min_y + tile_height;
      
      if (tile_x == tile_count_x - 1) {
        clip_rect.max_x = output_target->width;
      }
      if (tile_y == tile_count_y - 1) {
        clip_rect.max_y = output_target->height;
      }
      
      work->render_group = render_group;
      work->output_target = output_target;
      work->clip_rect = clip_rect;
      
      bool do_multithreaded_tiles = true;
      if (do_multithreaded_tiles)
        platform.add_entry(render_queue, do_tile_render_work, work);
      else
        do_tile_render_work(render_queue, work);
    }
  }
  
  platform.complete_all_work(render_queue);
}

internal
Render_group*
allocate_render_group(Game_asset_list *asset_list, Memory_arena* arena, u32 max_push_buffer_size, bool renders_in_background) {
  
  timed_function();
  
  Render_group* result = mem_push_struct(arena, Render_group);
  
  if (max_push_buffer_size == 0)
    max_push_buffer_size = (u32)get_arena_size_remaining(arena);
  
  result->push_buffer_base = (u8*)mem_push_size(arena, max_push_buffer_size);
  
  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;
  
  result->asset_list = asset_list;
  result->global_alpha = 1.0f;
  
  result->generation_id = 0;
  
  result->transform.offset_pos = {0,0,0};
  result->transform.scale = 1.0f;
  
  result->missing_resource_count = 0;
  result->renders_in_background = renders_in_background;
  
  result->inside_render = false;
  
  return result;
}

inline
void
perspective(Render_group *render_group, u32 pixel_width, u32 pixel_height, f32 meters_to_pixels, f32 focal_length, f32 dist_above_target) {
  
  f32 pixels_to_meters = safe_ratio_1(1.0f, meters_to_pixels);
  
  render_group->monitor_half_dim_meters = {
    0.5f * pixel_width * pixels_to_meters,
    0.5f * pixel_height * pixels_to_meters
  };
  
  render_group->transform.meters_to_pixels = meters_to_pixels;
  render_group->transform.focal_length = focal_length;
  render_group->transform.dist_above_target = dist_above_target;
  render_group->transform.screen_center = {
    0.5f * pixel_width,
    0.5f * pixel_height 
  };
  render_group->transform.orthographic = false;
}

inline
void
ortographic(Render_group *render_group, u32 pixel_width, u32 pixel_height, f32 meters_to_pixels) {
  
  f32 pixels_to_meters = safe_ratio_1(1.0f, meters_to_pixels);
  
  render_group->monitor_half_dim_meters = {
    0.5f * pixel_width * pixels_to_meters,
    0.5f * pixel_height * pixels_to_meters
  };
  
  render_group->transform.meters_to_pixels = meters_to_pixels;
  render_group->transform.focal_length = 1.0f;
  render_group->transform.dist_above_target = 1.0f;
  render_group->transform.screen_center = {
    0.5f * pixel_width,
    0.5f * pixel_height 
  };
  render_group->transform.orthographic = true;
}

inline
void
push_clear(Render_group *group, v4 color) {
  Render_entry_clear* entry = push_render_element(group, Render_entry_clear);
  
  if (!entry)
    return;
  
  entry->color = color;
}

inline
void
push_saturation(Render_group* group, f32 level) {
  Render_entry_saturation* entry = push_render_element(group, Render_entry_saturation);
  
  if (!entry)
    return;
  
  entry->level = level;
}

inline
void
push_coord_system(Render_group* group, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap, Loaded_bmp* normal_map, Env_map* top, Env_map* middle, Env_map* bottom) {
#if 0
  Entity_basis_result basis = get_render_entity_basis_pos(render_group, &entry->entity_basis, screen_dim);
  
  if (!basis.valid)
    return;
  
  Render_entry_coord_system *entry = push_render_element(group, Render_entry_coord_system);
  
  if (!entry)
    return;
  
  entry->origin = origin;
  entry->x_axis = x_axis;
  entry->y_axis = y_axis;
  entry->color = color;
  entry->bitmap = bitmap;
  entry->normal_map = normal_map;
  entry->top = top;
  entry->middle = middle;
  entry->bottom = bottom;
  
  return;
#endif
}

inline
v2
unproject(Render_group* render_group, v2 projected_xy, f32 at_dist_from_cam) {
  v2 raw_xy = (at_dist_from_cam / render_group->transform.focal_length) * projected_xy;
  
  return raw_xy;
}

inline
Rect2 
get_cam_rect_at_z(Render_group* group, f32 dist_from_cam) {
  
  Rect2 result;
  
  v2 raw_xy = unproject(group, group->monitor_half_dim_meters, dist_from_cam);
  
  result = rect_center_half_dim(v2{0,0}, raw_xy);
  
  return result;
}

inline
Rect2
get_cam_rect_at_target(Render_group* group) {
  Rect2 result;
  
  result = get_cam_rect_at_z(group, group->transform.dist_above_target);
  
  return result;
}

inline
bool
all_resources_present(Render_group *group) {
  bool result = (group->missing_resource_count == 0);
  
  return result;
}


static
void
begin_render(Render_group *group) {
  
  timed_function();
  
  if (group) {
    assert(!group->inside_render);
    group->inside_render = true;
    
    group->generation_id = begin_generation(group->asset_list);
  }
}

static
void
end_render(Render_group *group) {
  
  timed_function();
  
  if (group) {
    assert(group->inside_render);
    group->inside_render = false;
    
    end_generation(group->asset_list, group->generation_id);
    group->generation_id = 0;
    group->push_buffer_size = 0;
  }
}
