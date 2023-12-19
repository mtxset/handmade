#include "main.h"
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
srgb255_to_linear1(v4 color) {
  v4 result;
  
  f32 inv_255 = 1.0f / 255.0f;
  
  result.r = square(inv_255 * color.r);
  result.g = square(inv_255 * color.g);
  result.b = square(inv_255 * color.b);
  result.a = inv_255 * color.a;
  
  return result;
}

inline
v4
linear1_to_srgb255(v4 color) {
  v4 result;
  
  f32 one_255 = 255.0f;
  
  result.a = one_255 * color.a;
  result.r = one_255 * square_root(color.r);
  result.g = one_255 * square_root(color.g);
  result.b = one_255 * square_root(color.b);
  
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


#define push_render_element(group,type) (type*)push_render_element_(group, sizeof(type), Render_group_entry_type_##type)

inline
void*
push_render_element_(Render_group* group, u32 size, Render_group_entry_type type) {
  void* result = 0;
  
  size += sizeof(Render_group_entry_header);
  
  if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
    Render_group_entry_header* header = (Render_group_entry_header*)(group->push_buffer_base + group->push_buffer_size);
    header->type = type;
    result = (u8*)header + sizeof(*header);
    group->push_buffer_size += size;
  }
  else {
    macro_assert(!"INVALID");
  }
  
  return result;
}

inline
void
push_bitmap(Render_group* group, Loaded_bmp* bitmap, f32 height, v3 offset, v4 color = white_v4) {
  
  Render_entry_bitmap* entry = push_render_element(group, Render_entry_bitmap);
  
  if (!entry)
    return;
  
  entry->entity_basis.basis  = group->default_basis;
  entry->bitmap              = bitmap;
  
  v2 size = { 
    height * bitmap->width_over_height, 
    height
  };
  entry->size = size;
  
  v2 align = hadamard(bitmap->align_pcent, size);
  entry->entity_basis.offset = offset - v2_to_v3(align,0);
  entry->color               = group->global_alpha * color;
}

inline
void
push_rect(Render_group* group, v3 offset, v2 dim, v4 color = white_v4) {
  Render_entry_rect* piece = push_render_element(group, Render_entry_rect);
  
  if (!piece)
    return;
  
  v2 half_dim = 0.5f * dim;
  
  piece->entity_basis.basis     = group->default_basis;
  piece->entity_basis.offset    = offset - v2_to_v3(half_dim, 0);
  piece->color                  = color;
  piece->dim                    = dim;
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
  u32 lod_index = (u32)(roughness * (f32)(macro_array_count(map->lod) - 1) + 0.05f);
  macro_assert(lod_index < macro_array_count(map->lod));
  
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
  
  macro_assert(x >= 0 && x < lod->width);
  macro_assert(y >= 0 && y < lod->height);
  
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
debug_simd_example() {
  // https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
  __m128 xx = _mm_set1_ps(1.0f);
  __m128 yy = _mm_set1_ps(2.0f);
  __m128 sum = _mm_add_ps(xx, yy);
  
  __m128 xxxx = _mm_set_ps(1.0f, 2.0f, 30.0f, 40.0f); // will load in reverse
  __m128 yyyy = _mm_set_ps(1.0f, 2.0f, 30.0f, 40.0f); // 40.0f, 30.0f, 2.0f, 1.0f
  sum = _mm_add_ps(xxxx, yyyy);
}

#if 0
#include <iacaMarks.h>
#else
#define IACA_VC64_START
#define IACA_VC64_END
#endif

internal
void
draw_rect_quak(Loaded_bmp* buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap, f32 pixel_to_meter, Rect2i clip_rect, bool even) {
  u64 start_cycle_timer = __rdtsc();
  
  debug_simd_example();
  
  color.rgb *= color.a;
  
  f32 inverse_x_axis_squared = 1.0f / length_squared(x_axis);
  f32 inverse_y_axis_squared = 1.0f / length_squared(y_axis);
  
  u32 color_hex = (round_f32_u32(color.a * 255.0f) << 24 | 
                   round_f32_u32(color.r * 255.0f) << 16 | 
                   round_f32_u32(color.g * 255.0f) << 8  |
                   round_f32_u32(color.b * 255.0f) << 0);
  
  Rect2i fill_rect = rect_inverted_infinity();
  
  v2 p[4] = {
    origin,
    origin + x_axis,
    origin + x_axis + y_axis, 
    origin + y_axis
  };
  
  for (i32 i = 0; i < macro_array_count(p); i++) {
    v2 test_pos = p[i];
    
    int floor_x = floor_f32_i32(test_pos.x);
    int ceil_x  = ceil_f32_i32(test_pos.x) + 1;
    int floor_y = floor_f32_i32(test_pos.y);
    int ceil_y  = ceil_f32_i32(test_pos.y) + 1;
    
    if (fill_rect.min_x > floor_x) fill_rect.min_x = floor_x;
    if (fill_rect.min_y > floor_y) fill_rect.min_y = floor_y;
    if (fill_rect.max_x < ceil_x)  fill_rect.max_x = ceil_x;
    if (fill_rect.max_y < ceil_y)  fill_rect.max_y = ceil_y;
  }
  
  // multithreading - so we can split into scan lines for multiple threads 
  // and multiple regions 
  // scanlines are for cache locality in l1-l2 (considering 32kb) and hyperthreading (threading inside cpu pipes/units)
  // clipped rectangles are for threading on cores - giving different areas to render in parallel
  {
    //Rect2i clip_rect = { 128, 128, 256, 256 };
    fill_rect = rect_intersect(clip_rect, fill_rect);
    
    if (!even == (fill_rect.min_y & 1 )) {
      fill_rect.min_y += 1;
    }
  }
  
  if (!rect_has_area(fill_rect))
    return;
  
  // align into 16 bytes memory chunk
  __m128i startup_clip_mask = _mm_set1_epi32(0xffffffff);
  {
    i32 fill_width = fill_rect.max_x - fill_rect.min_x;
    i32 fill_width_align = fill_width & 3;
    
    if (fill_width_align > 0) {
      i32 adjust = 4 - fill_width_align;
      switch (adjust) {
        case 1: startup_clip_mask = _mm_slli_si128(startup_clip_mask, 1 * 4); break;
        case 2: startup_clip_mask = _mm_slli_si128(startup_clip_mask, 2 * 4); break;
        case 3: startup_clip_mask = _mm_slli_si128(startup_clip_mask, 3 * 4); break;
      }
      fill_width += adjust;
      fill_rect.min_x = fill_rect.max_x - fill_width;
    }
  }
  
  f32 one_255 = 255.0f;
  f32 inv_255 = 1.0f / 255.0f;
  
  v2 nx_axis = inverse_x_axis_squared * x_axis;
  v2 ny_axis = inverse_y_axis_squared * y_axis;
  
  __m128 zero_4x = _mm_set1_ps(0.0f);
  __m128 one_4x = _mm_set1_ps(1.0f);
  __m128 four_4x = _mm_set1_ps(4.0f);
  __m128 one_255_4x = _mm_set1_ps(255.0f);
  __m128 inv_255_4x = _mm_set1_ps(inv_255);
  
  __m128i mask_ff = _mm_set1_epi32(0xff);
  __m128i mask_ff00ff = _mm_set1_epi32(0x00ff00ff);
  
  __m128 origin_x_4x = _mm_set1_ps(origin.x);
  __m128 origin_y_4x = _mm_set1_ps(origin.y);
  
  __m128 nx_axisx_4x = _mm_set1_ps(nx_axis.x);
  __m128 nx_axisy_4x = _mm_set1_ps(nx_axis.y);
  __m128 ny_axisx_4x = _mm_set1_ps(ny_axis.x);
  __m128 ny_axisy_4x = _mm_set1_ps(ny_axis.y);
  
  __m128 width_m2  = _mm_set1_ps((f32)(bitmap->width  - 2));
  __m128 height_m2 = _mm_set1_ps((f32)(bitmap->height - 2));
  
  i32 bitmap_pitch = bitmap->pitch;
  void* bitmap_memory = bitmap->memory;
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8* row = (u8*)buffer->memory + fill_rect.min_x * bytes_per_pixel + fill_rect.min_y * buffer->pitch;
  i32 row_advance = 2 * buffer->pitch;
  
  i32 min_x = fill_rect.min_x;
  i32 min_y = fill_rect.min_y;
  i32 max_x = fill_rect.max_x;
  i32 max_y = fill_rect.max_y;
  
  u64 pixel_cycle_timer = __rdtsc();
  for (i32 y = min_y; y < max_y; y += 2) {
    
    __m128 pixel_pos_y = _mm_set1_ps((f32)y);
    pixel_pos_y = _mm_sub_ps(pixel_pos_y, origin_y_4x);
    
    __m128 pixel_pos_x = _mm_set_ps((f32)(min_x + 3), 
                                    (f32)(min_x + 2), 
                                    (f32)(min_x + 1), 
                                    (f32)(min_x + 0));
    pixel_pos_x = _mm_sub_ps(pixel_pos_x, origin_x_4x);
    
    __m128i clip_mask = startup_clip_mask;
    
    u32* pixel = (u32*)row;
    for (i32 x = min_x; x < max_x; x += 4) {
      
      IACA_VC64_START;
      __m128 u = _mm_add_ps(_mm_mul_ps(pixel_pos_x, nx_axisx_4x), _mm_mul_ps(pixel_pos_y, nx_axisy_4x));
      __m128 v = _mm_add_ps(_mm_mul_ps(pixel_pos_x, ny_axisx_4x), _mm_mul_ps(pixel_pos_y, ny_axisy_4x));
      
      __m128i write_mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(u, zero_4x),
                                                                  _mm_cmple_ps(u, one_4x)),
                                                       _mm_and_ps(_mm_cmpge_ps(v, zero_4x),
                                                                  _mm_cmple_ps(v, one_4x))));
      
      write_mask = _mm_and_si128(write_mask, clip_mask);
      
      __m128i original_dest = _mm_loadu_si128((__m128i*) pixel);
      
      u = _mm_min_ps(_mm_max_ps(u, zero_4x), one_4x);
      v = _mm_min_ps(_mm_max_ps(v, zero_4x), one_4x);
      
      __m128 t_x = _mm_mul_ps(u, width_m2);
      __m128 t_y = _mm_mul_ps(v, height_m2);
      
      __m128i fetch_x_4x = _mm_cvttps_epi32(t_x);
      __m128i fetch_y_4x = _mm_cvttps_epi32(t_y);
      
      __m128 f_x = _mm_sub_ps(t_x, _mm_cvtepi32_ps(fetch_x_4x));
      __m128 f_y = _mm_sub_ps(t_y, _mm_cvtepi32_ps(fetch_y_4x));
      
      // fetch samples
      __m128i samplea, sampleb, samplec, sampled;
      {
        i32 fetch_x0 = fetch_x_4x.m128i_i32[0];
        i32 fetch_y0 = fetch_y_4x.m128i_i32[0];
        i32 fetch_x1 = fetch_x_4x.m128i_i32[1];
        i32 fetch_y1 = fetch_y_4x.m128i_i32[1];
        i32 fetch_x2 = fetch_x_4x.m128i_i32[2];
        i32 fetch_y2 = fetch_y_4x.m128i_i32[2];
        i32 fetch_x3 = fetch_x_4x.m128i_i32[3];
        i32 fetch_y3 = fetch_y_4x.m128i_i32[3];
        
        size_t u32_size = sizeof(u32);
        u8* texel_ptr0 = ((u8*)bitmap_memory) + fetch_y0 * bitmap_pitch + fetch_x0 * u32_size;
        u8* texel_ptr1 = ((u8*)bitmap_memory) + fetch_y1 * bitmap_pitch + fetch_x1 * u32_size;
        u8* texel_ptr2 = ((u8*)bitmap_memory) + fetch_y2 * bitmap_pitch + fetch_x2 * u32_size;
        u8* texel_ptr3 = ((u8*)bitmap_memory) + fetch_y3 * bitmap_pitch + fetch_x3 * u32_size;
        
        samplea = _mm_setr_epi32(*(u32*)(texel_ptr0),
                                 *(u32*)(texel_ptr1),
                                 *(u32*)(texel_ptr2),
                                 *(u32*)(texel_ptr3));
        
        sampleb = _mm_setr_epi32(*(u32*)(texel_ptr0 + u32_size),
                                 *(u32*)(texel_ptr1 + u32_size),
                                 *(u32*)(texel_ptr2 + u32_size),
                                 *(u32*)(texel_ptr3 + u32_size));
        
        samplec = _mm_setr_epi32(*(u32*)(texel_ptr0 + bitmap_pitch),
                                 *(u32*)(texel_ptr1 + bitmap_pitch),
                                 *(u32*)(texel_ptr2 + bitmap_pitch),
                                 *(u32*)(texel_ptr3 + bitmap_pitch));
        
        sampled = _mm_setr_epi32(*(u32*)(texel_ptr0 + bitmap_pitch + u32_size),
                                 *(u32*)(texel_ptr1 + bitmap_pitch + u32_size),
                                 *(u32*)(texel_ptr2 + bitmap_pitch + u32_size),
                                 *(u32*)(texel_ptr3 + bitmap_pitch + u32_size));
      }
      
      // unpacking samples
      __m128 texel_a_r, texel_a_g, texel_a_b, texel_a_a;
      __m128 texel_b_r, texel_b_g, texel_b_b, texel_b_a;
      __m128 texel_c_r, texel_c_g, texel_c_b, texel_c_a;
      __m128 texel_d_r, texel_d_g, texel_d_b, texel_d_a;
      {
        texel_a_b = _mm_cvtepi32_ps(_mm_and_si128(samplea, mask_ff));
        texel_a_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplea, 8), mask_ff));
        texel_a_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplea, 16), mask_ff));
        texel_a_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplea, 24), mask_ff));
        
        texel_b_b = _mm_cvtepi32_ps(_mm_and_si128(sampleb, mask_ff));
        texel_b_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleb, 8), mask_ff));
        texel_b_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleb, 16), mask_ff));
        texel_b_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampleb, 24), mask_ff));
        
        texel_c_b = _mm_cvtepi32_ps(_mm_and_si128(samplec, mask_ff));
        texel_c_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplec, 8), mask_ff));
        texel_c_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplec, 16), mask_ff));
        texel_c_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(samplec, 24), mask_ff));
        
        texel_d_b = _mm_cvtepi32_ps(_mm_and_si128(sampled, mask_ff));
        texel_d_g = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampled, 8), mask_ff));
        texel_d_r = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampled, 16), mask_ff));
        texel_d_a = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(sampled, 24), mask_ff));
      }
      
      // load destination
      __m128 dstr, dstg, dstb, dsta;
      {
        dstb = _mm_cvtepi32_ps(_mm_and_si128(original_dest, mask_ff));
        dstg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 8), mask_ff));
        dstr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 16), mask_ff));
        dsta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(original_dest, 24), mask_ff));
      }
      
#define mm_square(a) _mm_mul_ps(a, a)
      // convert from srgb255_to_linear1 
      {
        texel_a_r = mm_square(texel_a_r);
        texel_a_g = mm_square(texel_a_g);
        texel_a_b = mm_square(texel_a_b);
        
        texel_b_r = mm_square(texel_b_r);
        texel_b_g = mm_square(texel_b_g);
        texel_b_b = mm_square(texel_b_b);
        
        texel_c_r = mm_square(texel_c_r);
        texel_c_g = mm_square(texel_c_g);
        texel_c_b = mm_square(texel_c_b);
        
        texel_d_r = mm_square(texel_d_r);
        texel_d_g = mm_square(texel_d_g);
        texel_d_b = mm_square(texel_d_b);
      }
      
      // lerp
      __m128 texelr, texelg, texelb, texela;
      {
        __m128 if_x = _mm_sub_ps(one_4x, f_x);
        __m128 if_y = _mm_sub_ps(one_4x, f_y);
        
        __m128 l0 = _mm_mul_ps(if_y, if_x);
        __m128 l1 = _mm_mul_ps(if_y, f_x);
        __m128 l2 = _mm_mul_ps(f_y, if_x);
        __m128 l3 = _mm_mul_ps(f_y, f_x);
        
        texelr = _mm_add_ps(_mm_add_ps(_mm_add_ps
                                       (_mm_mul_ps(l0, texel_a_r), _mm_mul_ps(l1, texel_b_r)), _mm_mul_ps(l2, texel_c_r)), _mm_mul_ps(l3, texel_d_r));
        
        texelg = _mm_add_ps(_mm_add_ps(_mm_add_ps
                                       (_mm_mul_ps(l0, texel_a_g), _mm_mul_ps(l1, texel_b_g)), _mm_mul_ps(l2, texel_c_g)), _mm_mul_ps(l3, texel_d_g));
        
        texelb = _mm_add_ps(_mm_add_ps(_mm_add_ps
                                       (_mm_mul_ps(l0, texel_a_b), _mm_mul_ps(l1, texel_b_b)), _mm_mul_ps(l2, texel_c_b)), _mm_mul_ps(l3, texel_d_b));
        
        texela = _mm_add_ps(_mm_add_ps(_mm_add_ps
                                       (_mm_mul_ps(l0, texel_a_a), _mm_mul_ps(l1, texel_b_a)), _mm_mul_ps(l2, texel_c_a)), _mm_mul_ps(l3, texel_d_a));
      }
      
      // multiple by incoming color (gamma correction)
      
      {
        __m128 color_r_4x = _mm_set1_ps(color.r);
        __m128 color_g_4x = _mm_set1_ps(color.g);
        __m128 color_b_4x = _mm_set1_ps(color.b);
        __m128 color_a_4x = _mm_set1_ps(color.a);
        
        texelr = _mm_mul_ps(texelr, color_r_4x);
        texelg = _mm_mul_ps(texelg, color_g_4x);
        texelb = _mm_mul_ps(texelb, color_b_4x);
        texela = _mm_mul_ps(texela, color_a_4x);
      }
      
      // clamp
      {
        __m128 max_color_value = _mm_set1_ps(255.0f * 255.0f);
        texelr = _mm_min_ps(_mm_max_ps(texelr, zero_4x), max_color_value);
        texelr = _mm_min_ps(_mm_max_ps(texelr, zero_4x), max_color_value);
        texelr = _mm_min_ps(_mm_max_ps(texelr, zero_4x), max_color_value);
      }
      
      // converting back from srgb to linear
      {
        dstr = mm_square(dstr);
        dstg = mm_square(dstg);
        dstb = mm_square(dstb);
        // dsta = dsta;
      }
      
      // (1-t)A + tB 
      // alpha (0 - 1);
      // color = dst_color + alpha (src_color - dst_color)
      // color = (1 - alpha)dst_color + alpha * src_color;
      
      // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
      
      // linear blending
      __m128 blendedr, blendedg, blendedb, blendeda;
      {
        __m128 inv_texel_a = _mm_sub_ps(one_4x, _mm_mul_ps(inv_255_4x, texela));
        blendedr = _mm_add_ps(_mm_mul_ps(inv_texel_a, dstr), texelr);
        blendedg = _mm_add_ps(_mm_mul_ps(inv_texel_a, dstg), texelg);
        blendedb = _mm_add_ps(_mm_mul_ps(inv_texel_a, dstb), texelb);
        blendeda = _mm_add_ps(_mm_mul_ps(inv_texel_a, dsta), texela);
      }
      
      // back to srgb
      {
#if 0
        blendedr = _mm_sqrt_ps(blendedr);
        blendedg = _mm_sqrt_ps(blendedg);
        blendedb = _mm_sqrt_ps(blendedb);
#else
        blendedr = _mm_mul_ps(blendedr, _mm_rsqrt_ps(blendedr));
        blendedg = _mm_mul_ps(blendedg, _mm_rsqrt_ps(blendedg));
        blendedb = _mm_mul_ps(blendedb, _mm_rsqrt_ps(blendedb));
#endif
        //blendeda = blendeda;
      }
      
      // convert float values to integer
      __m128i out = {};
      {
        __m128i i32_r = _mm_cvtps_epi32(blendedr);
        __m128i i32_g = _mm_cvtps_epi32(blendedg);
        __m128i i32_b = _mm_cvtps_epi32(blendedb);
        __m128i i32_a = _mm_cvtps_epi32(blendeda);
        
        __m128i shifted_r = _mm_slli_epi32(i32_r, 16);
        __m128i shifted_g = _mm_slli_epi32(i32_g, 8);
        __m128i shifted_b = _mm_slli_epi32(i32_b, 0);
        __m128i shifted_a = _mm_slli_epi32(i32_a, 24);
        
        out = _mm_or_si128(_mm_or_si128(shifted_r, shifted_g), _mm_or_si128(shifted_b, shifted_a));
      }
      
      __m128i masket_out = _mm_or_si128(_mm_and_si128(write_mask, out),
                                        _mm_andnot_si128(write_mask, original_dest));
      
      // to bypass alignent in 16 bytes
      _mm_storeu_si128((__m128i *)pixel, masket_out);
      
      pixel_pos_x = _mm_add_ps(pixel_pos_x, four_4x); 
      pixel += 4;
      clip_mask = _mm_set1_epi32(0xffffffff);
    }
    IACA_VC64_END;
    row += row_advance;
  }
  
  i32 pixels_processed = rect_clamp_area(fill_rect) / 2;
  
  debug_end_timer(Debug_cycle_counter_type_process_pixel, pixel_cycle_timer, pixels_processed);
  
  debug_end_timer(Debug_cycle_counter_type_render_draw_rect_quak, start_cycle_timer);
}

internal
void
draw_rect_slow(Loaded_bmp* buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap, Loaded_bmp* normal_map, Env_map* top, Env_map* middle, Env_map* bottom, f32 pixel_to_meter) {
  u64 start_cycle_timer = __rdtsc();
  
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
  
  for (i32 i = 0; i < macro_array_count(p); i++) {
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
        
        //macro_assert(u >= 0.0f && u <= 1.0f);
        //macro_assert(v >= 0.0f && v <= 1.0f);
        
        f32 t_x = (u * ((f32)(bitmap->width - 2)));
        f32 t_y = (v * ((f32)(bitmap->height - 2)));
        
        i32 x_i32 = (i32)t_x;
        i32 y_i32 = (i32)t_y;
        
        f32 f_x = t_x - (f32)x_i32;
        f32 f_y = t_y - (f32)y_i32;
        
        macro_assert(x_i32 >= 0 && x_i32 < bitmap->width);
        macro_assert(y_i32 >= 0 && y_i32 < bitmap->height);
        
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
  
  u32 pixels_processed = 0;
  
  if (x_max >= x_min && y_max >= y_min)
    pixels_processed = (x_max - x_min + 1) * (y_max - y_min + 1);
  
  debug_end_timer(Debug_cycle_counter_type_process_pixel, pixel_cycle_timer, pixels_processed);
  
  debug_end_timer(Debug_cycle_counter_type_render_draw_rect_slow, start_cycle_timer);
}

internal
void
change_saturation(Loaded_bmp* bitmap_buffer, f32 level) {
  
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
  macro_assert(source_row);
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

struct Entity_basis_result {
  v2 pos;
  f32 scale;
  bool valid;
};

inline
Entity_basis_result
get_render_entity_basis_pos(Render_group* render_group, Render_entity_basis* basis, v2 screen_dim) {
  Entity_basis_result result = {};
  
  f32 meters_to_pixels = render_group->meters_to_pixels;
  
  v2 screen_center = 0.5 * screen_dim;
  
  v3 entity_base_pos = basis->basis->position;
  
  f32 dist_to_p = render_group->render_camera.dist_above_target - entity_base_pos.z;
  f32 near_clip_plane = 0.2f;
  
  v3 raw_xy = v2_to_v3(entity_base_pos.xy + basis->offset.xy, 1.0f);
  
  if (dist_to_p > near_clip_plane) {
    v3 projected_xy = (1.0f / dist_to_p) * render_group->render_camera.focal_length * raw_xy;
    
    result.pos = screen_center + meters_to_pixels * projected_xy.xy;
    result.scale = meters_to_pixels * projected_xy.z;
    result.valid = true;
  }
  
  return result;
}

internal
void
render_group_to_output(Render_group* render_group, Loaded_bmp* output_target, Rect2i clip_rect, bool even) {
  
  auto start_cycle_count = __rdtsc();
  
  v2 screen_dim = { 
    (f32)output_target->width,
    (f32)output_target->height
  };
  
  f32 pixel_to_meter = 1.0f / render_group->meters_to_pixels;
  
  for (u32 base_addr = 0; base_addr < render_group->push_buffer_size;) {
    
    Render_group_entry_header* header = (Render_group_entry_header*)(render_group->push_buffer_base + base_addr);
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
        base_addr += sizeof(*entry);
        
        Entity_basis_result basis = get_render_entity_basis_pos(render_group, &entry->entity_basis, screen_dim);
        macro_assert(entry->bitmap);
#if 0
        draw_rect_slow(output_target, basis.pos, 
                       basis.scale * v2{entry->size.x, 0},
                       basis.scale * v2{0, entry->size.y},
                       entry->color,
                       entry->bitmap, 0,0,0,0, pixel_to_meter);
#else
        draw_rect_quak(output_target, basis.pos, 
                       basis.scale * v2{entry->size.x, 0},
                       basis.scale * v2{0, entry->size.y},
                       entry->color,
                       entry->bitmap, pixel_to_meter, 
                       clip_rect, even);
#endif
        
      } break;
      
      case Render_group_entry_type_Render_entry_rect: {
        Render_entry_rect* entry = (Render_entry_rect*)data;
        base_addr += sizeof(*entry);
        
        Entity_basis_result basis = get_render_entity_basis_pos(render_group, &entry->entity_basis, screen_dim);
        
        draw_rect(output_target, basis.pos, basis.pos + basis.scale * entry->dim, entry->color, clip_rect, even);
      } break;
      
      case Render_group_entry_type_Render_entry_coord_system: {
        Render_entry_coord_system* entry = (Render_entry_coord_system*)data;
        base_addr += sizeof(*entry);
        
        v2 pos = entry->origin;
        v2 dim = v2{2,2};
        
        v2 end = entry->origin + entry->x_axis + entry->y_axis;
        draw_rect_slow(output_target, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->bitmap, entry->normal_map, entry->top, entry->middle, entry->bottom, pixel_to_meter);
        
        v4 corner_color = yellow_v4;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        pos = entry->origin + entry->x_axis;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        pos = entry->origin + entry->y_axis;
        draw_rect(output_target, pos - dim, pos + dim, corner_color, clip_rect, even);
        
        draw_rect(output_target, end - dim, end + dim, corner_color, clip_rect, even);
#if 0
        for (u32 i = 0; i < macro_array_count(entry->points); i++) {
          v2 p = entry->points[i];
          p = entry->origin + p.x * entry->x_axis + p.y * entry->y_axis;
          draw_rect(output_target, p - dim, p + dim, corner_color);
        }
#endif
      } break;
      
      default: {
        macro_assert(!"CRASH");
      } break;
    }
  }
  
  debug_end_timer(Debug_cycle_counter_type_render_group_to_output, start_cycle_count);
}

struct Tile_render_work
{
  Render_group* render_group;
  Loaded_bmp *output_target;
  Rect2i clip_rect;
};

internal void
do_tile_render_work(void* data)
{
  Tile_render_work* work = (Tile_render_work*)data;
  
  bool even = true;
  bool not_even = false;
  
  render_group_to_output(work->render_group, work->output_target, work->clip_rect, even);
  render_group_to_output(work->render_group, work->output_target, work->clip_rect, not_even);
}

internal
void
tiled_render_group_to_output(
                             //Work_queue* render_queue, 
                             Render_group* render_group, Loaded_bmp* output_target) {
  
  i32 const tile_count_x = 4;
  i32 const tile_count_y = 4;
  Tile_render_work work_array[tile_count_y * tile_count_x];
  
  int tile_width = output_target->width / tile_count_x;
  int tile_height = output_target->height / tile_count_y;
  
  
  i32 work_count = 0;
  for (i32 tile_y = 0; tile_y < tile_count_y; tile_y++) {
    for (i32 tile_x = 0; tile_x < tile_count_x; tile_x++) {
      
      Tile_render_work* work = work_array + work_count++;
      
      Rect2i clip_rect;
      
      clip_rect.min_x = tile_x * tile_width  + 4;
      clip_rect.min_y = tile_y * tile_height + 4;
      
      clip_rect.max_x = clip_rect.min_x + tile_width  - 4;
      clip_rect.max_y = clip_rect.min_y + tile_height - 4;
      
      
      work->render_group = render_group;
      work->output_target = output_target;
      work->clip_rect = clip_rect;
      
      //add_entry(render_queue, do_tile_render_work, work);
    }
  }
  
  //complete_all_work(render_queue);
  
  for (i32 work_index = 0; work_index < work_count; work_index++) {
    Tile_render_work* work = work_array + work_index;
    do_tile_render_work(work);
  }
  
}

internal
Render_group*
allocate_render_group(Memory_arena* arena, u32 max_push_buffer_size, u32 resolution_pixels_x, u32 resolution_pixels_y) {
  Render_group* result = mem_push_struct(arena, Render_group);
  result->push_buffer_base = (u8*)mem_push_size(arena, max_push_buffer_size);
  
  result->default_basis = mem_push_struct(arena, Render_basis);
  result->default_basis->position = v3{0,0,0};
  
  result->max_push_buffer_size = max_push_buffer_size;
  result->push_buffer_size = 0;
  
  result->game_camera.focal_length = 0.6f; // actual meters eye is from the monitor
  result->game_camera.dist_above_target = 9.0f;
  
  result->render_camera = result->game_camera;
  // result->render_camera.dist_above_target = 30.0f;
  
  result->global_alpha = 1.0f;
  
  f32 width_of_monitor = 0.635f;
  result->meters_to_pixels = (f32)resolution_pixels_x * width_of_monitor;
  
  f32 pixel_to_meter = 1.0f / result->meters_to_pixels;
  result->monitor_half_dim_meters = {
    0.5f * resolution_pixels_x * pixel_to_meter,
    0.5f * resolution_pixels_y * pixel_to_meter
  };
  return result;
}

inline
void
push_clear(Render_group* group, v4 color) {
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
Render_entry_coord_system*
push_coord_system(Render_group* group, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap, Loaded_bmp* normal_map, Env_map* top, Env_map* middle, Env_map* bottom) {
  Render_entry_coord_system* entry = push_render_element(group, Render_entry_coord_system);
  
  if (!entry)
    return entry;
  
  entry->origin = origin;
  entry->x_axis = x_axis;
  entry->y_axis = y_axis;
  entry->color = color;
  entry->bitmap = bitmap;
  entry->normal_map = normal_map;
  entry->top = top;
  entry->middle = middle;
  entry->bottom = bottom;
  
  return entry;
}

inline
v2
unproject(Render_group* render_group, v2 projected_xy, f32 at_dist_from_cam) {
  v2 raw_xy = (at_dist_from_cam / render_group->game_camera.focal_length) * projected_xy;
  
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
  
  result = get_cam_rect_at_z(group, group->game_camera.dist_above_target);
  
  return result;
}