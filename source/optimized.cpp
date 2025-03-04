
#include "game.h"

#if !defined(internal)
#define internal
#endif

#if 0
#include <iacaMarks.h>
#else
#define IACA_VC64_START
#define IACA_VC64_END
#endif

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

inline
i32
get_clamped_rect_area(Rect2i rect) {
  i32 width  = (rect.max_x - rect.min_x);
  i32 height = (rect.max_y - rect.min_y);
  
  i32 result = 0;
  
  if (width > 0 && height > 0) 
    result = width * height;
  
  return result;
}

void
draw_rect_quak(Loaded_bmp *buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp *bitmap, f32 pixel_to_meter, Rect2i clip_rect, bool even) {
  
  timed_block();
  
  assert(bitmap->memory);
  
  color.rgb *= color.a;
  
  f32 inverse_x_axis_squared = 1.0f / length_squared(x_axis);
  f32 inverse_y_axis_squared = 1.0f / length_squared(y_axis);
  
  Rect2i fill_rect = rect_inverted_infinity();
  
  v2 p[4] = {
    origin,
    origin + x_axis,
    origin + x_axis + y_axis, 
    origin + y_axis
  };
  
  for (i32 i = 0; i < array_count(p); i++) {
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
  
  __m128i start_clip_mask = _mm_set1_epi8(-1);
  __m128i end_clip_mask = _mm_set1_epi8(-1);
  
  __m128i start_clip_masks[] = {
    _mm_slli_si128(start_clip_mask, 0 * 4),
    _mm_slli_si128(start_clip_mask, 1 * 4),
    _mm_slli_si128(start_clip_mask, 2 * 4),
    _mm_slli_si128(start_clip_mask, 3 * 4),
  };
  
  __m128i end_clip_masks[] = {
    _mm_srli_si128(end_clip_mask, 0 * 4),
    _mm_srli_si128(end_clip_mask, 3 * 4),
    _mm_srli_si128(end_clip_mask, 2 * 4),
    _mm_srli_si128(end_clip_mask, 1 * 4),
  };
  
  if (fill_rect.min_x & 3) {
    start_clip_mask = start_clip_masks[fill_rect.min_x & 3];
    fill_rect.min_x = fill_rect.min_x & ~3;
  }
  
  if (fill_rect.max_x & 3) {
    end_clip_mask = end_clip_masks[fill_rect.max_x & 3];
    fill_rect.max_x = (fill_rect.max_x & ~3) + 4;
  }
  
  /*
  // align into 16 bytes memory chunk
  __m128i startup_clip_mask = _mm_set1_epi32(0xffffffff);
  {
    i32 fill_width = fill_rect.max_x - fill_rect.min_x;
    i32 fill_width_align = fill_width & 3;
    
    if (fill_width_align > 0) {
      i32 adjust = 4 - fill_width_align;
      switch (adjust) {
        
      }
      fill_width += adjust;
      fill_rect.min_x = fill_rect.max_x - fill_width;
    }
  }
  */
  
  f32 inv_255 = 1.0f / 255.0f;
  
  v2 nx_axis = inverse_x_axis_squared * x_axis;
  v2 ny_axis = inverse_y_axis_squared * y_axis;
  
  __m128 zero_4x = _mm_set1_ps(0.0f);
  __m128 one_4x = _mm_set1_ps(1.0f);
  __m128 half_4x = _mm_set1_ps(0.5f);
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
  void *bitmap_memory = bitmap->memory;
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  u8 *row = (u8*)buffer->memory + fill_rect.min_x * bytes_per_pixel + fill_rect.min_y * buffer->pitch;
  i32 row_advance = 2 * buffer->pitch;
  
  i32 min_x = fill_rect.min_x;
  i32 min_y = fill_rect.min_y;
  i32 max_x = fill_rect.max_x;
  i32 max_y = fill_rect.max_y;
  
  //timed_block(get_clamped_rect_area(fill_rect) / 2);
  for (i32 y = min_y; y < max_y; y += 2) {
    
    __m128 pixel_pos_y = _mm_set1_ps((f32)y);
    pixel_pos_y = _mm_sub_ps(pixel_pos_y, origin_y_4x);
    
    __m128 pixel_pos_x = _mm_set_ps((f32)(min_x + 3), 
                                    (f32)(min_x + 2), 
                                    (f32)(min_x + 1), 
                                    (f32)(min_x + 0));
    pixel_pos_x = _mm_sub_ps(pixel_pos_x, origin_x_4x);
    
    __m128i clip_mask = start_clip_mask;
    
    u32 *pixel = (u32*)row;
    for (i32 x = min_x; x < max_x; x += 4) {
      
      IACA_VC64_START;
      __m128 u = _mm_add_ps(_mm_mul_ps(pixel_pos_x, nx_axisx_4x), _mm_mul_ps(pixel_pos_y, nx_axisy_4x));
      __m128 v = _mm_add_ps(_mm_mul_ps(pixel_pos_x, ny_axisx_4x), _mm_mul_ps(pixel_pos_y, ny_axisy_4x));
      
      __m128i write_mask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(u, zero_4x),
                                                                  _mm_cmple_ps(u, one_4x)),
                                                       _mm_and_ps(_mm_cmpge_ps(v, zero_4x),
                                                                  _mm_cmple_ps(v, one_4x))));
      
      write_mask = _mm_and_si128(write_mask, clip_mask);
      
      __m128i original_dest = _mm_load_si128((__m128i*) pixel);
      
      u = _mm_min_ps(_mm_max_ps(u, zero_4x), one_4x);
      v = _mm_min_ps(_mm_max_ps(v, zero_4x), one_4x);
      
      __m128 t_x = _mm_add_ps(_mm_mul_ps(u, width_m2), half_4x);
      __m128 t_y = _mm_add_ps(_mm_mul_ps(v, height_m2), half_4x);
      
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
      _mm_store_si128((__m128i *)pixel, masket_out);
      
      pixel_pos_x = _mm_add_ps(pixel_pos_x, four_4x); 
      pixel += 4;
      
      if (x + 8 < max_x) {
        clip_mask = _mm_set1_epi32(0xffffffff);
      }
      else {
        clip_mask = end_clip_mask;
      }
      
    }
    IACA_VC64_END;
    row += row_advance;
  }
}

extern u32 const optimized_debug_record_list_count = __COUNTER__;
Debug_record optimized_debug_record_list[optimized_debug_record_list_count];
