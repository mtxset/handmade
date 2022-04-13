#include "main.h"
#include "color.h"

#define push_render_element(group,type) (type*)push_render_element_(group, sizeof(type), Render_group_entry_type_##type)

inline
Render_group_entry_header*
push_render_element_(Render_group* group, u32 size, Render_group_entry_type type) {
    Render_group_entry_header* result = 0;
    
    if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
        result = (Render_group_entry_header*)(group->push_buffer_base + group->push_buffer_size);
        result->type = type;
        group->push_buffer_size += size;
    }
    else {
        macro_assert(!"INVALID");
    }
    
    return result;
}

inline
void
push_piece(Render_group* group, Loaded_bmp* bitmap, 
           v2 offset, f32 offset_z, v2 align, v2 dim, v4 color, f32 entity_zc) {
    
    Render_entry_bitmap* piece = push_render_element(group, Render_entry_bitmap);
    
    if (!piece)
        return;
    
    piece->entity_basis.basis     = group->default_basis;
    piece->bitmap                 = bitmap;
    piece->entity_basis.offset    = group->meters_to_pixels * v2 { offset.x, -offset.y } - align;
    piece->entity_basis.offset_z  = offset_z;
    piece->entity_basis.entity_zc = entity_zc;
    piece->color                  = color;
}

inline
void
push_bitmap(Render_group* group, Loaded_bmp* bitmap, 
            v2 offset, f32 offset_z, v2 align, f32 alpha = 1.0f, f32 entity_zc = 1.0f) {
    push_piece(group, bitmap, offset, offset_z, align, v2 {0, 0}, v4 {1.0f, 1.0f, 1.0f, alpha }, entity_zc);
}

inline
void
push_rect(Render_group* group, v2 offset, f32 offset_z, v2 dim, v4 color, f32 entity_zc = 1.0f) {
    Render_entry_rect* piece = push_render_element(group, Render_entry_rect);
    
    if (!piece)
        return;
    
    v2 half_dim = 0.5f * group->meters_to_pixels * dim;
    
    piece->entity_basis.basis     = group->default_basis;
    piece->entity_basis.offset    = group->meters_to_pixels
        * v2 { offset.x, -offset.y } - half_dim;// v2 {-half_dim.x, half_dim.y};
    
    piece->entity_basis.offset_z  = offset_z;
    piece->entity_basis.entity_zc = entity_zc;
    piece->color                  = color;
    piece->dim                    = group->meters_to_pixels * dim;;
}

inline
void
push_rect_outline(Render_group* group, v2 offset, f32 offset_z, v2 dim, v4 color, f32 entity_zc = 1.0f) {
    
    f32 thickness = 0.1f;
    push_rect(group, offset - v2{0, dim.y/2}, offset_z, v2{dim.x, thickness}, color, entity_zc);
    push_rect(group, offset + v2{0, dim.y/2}, offset_z, v2{dim.x, thickness}, color, entity_zc);
    
    push_rect(group, offset - v2{dim.x/2, 0}, offset_z, v2{thickness, dim.y}, color, entity_zc);
    push_rect(group, offset + v2{dim.x/2, 0}, offset_z, v2{thickness, dim.y}, color, entity_zc);
}

internal
void
draw_rect(Loaded_bmp* buffer, v2 start, v2 end, v4 color) {
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
draw_rect_slow(Loaded_bmp* buffer, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap) {
    
    u32 color_hex = 0;
    
    color_hex = (round_f32_u32(color.a * 255.0f) << 24 | 
                 round_f32_u32(color.r * 255.0f) << 16 | 
                 round_f32_u32(color.g * 255.0f) << 8  |
                 round_f32_u32(color.b * 255.0f) << 0);
    
    v2 p[4] = {
        origin,
        origin + x_axis,
        origin + x_axis + y_axis,
        origin + y_axis
    };
    
    i32 width_max = buffer->width - 1;
    i32 height_max = buffer->height - 1;
    
    i32 y_min = height_max;
    i32 y_max = 0;
    i32 x_min = width_max;
    i32 x_max = 0;
    
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
    
    f32 inverse_x_axis_squared = 1.0f / length_squared(x_axis);
    f32 inverse_y_axis_squared = 1.0f / length_squared(y_axis);
    
    for (i32 y = y_min; y < y_max; y++) {
        u32* pixel = (u32*)row;
        for (i32 x = x_min; x < x_max; x++) {
            
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
                
                f32 u = inverse_x_axis_squared * inner(d, x_axis);
                f32 v = inverse_y_axis_squared * inner(d, y_axis);
                
                macro_assert(u >= 0.0f && u <= 1.0f);
                macro_assert(v >= 0.0f && v <= 1.0f);
                
                f32 x_f32 = (u * ((f32)(bitmap->width - 2)));
                f32 y_f32 = (v * ((f32)(bitmap->height - 2)));
                
                i32 x_i32 = (i32)x_f32;
                i32 y_i32 = (i32)y_f32;
                
                f32 f_x = x_f32 - (f32)x_i32;
                f32 f_y = y_f32 - (f32)y_i32;
                
                macro_assert(x_i32 >= 0 && x_i32 < bitmap->width);
                macro_assert(y_i32 >= 0 && y_i32 < bitmap->height);
                
                u8* texel_ptr = (u8*)bitmap->memory + y_i32 * bitmap->pitch + x_i32 * sizeof(u32);
                v4 texel = {};
                // blending pixels
                {
                    u32 texel_ptr_a = *(u32*)texel_ptr;
                    u32 texel_ptr_b = *(u32*)(texel_ptr + sizeof(u32));
                    u32 texel_ptr_c = *(u32*)(texel_ptr + bitmap->pitch);
                    u32 texel_ptr_d = *(u32*)(texel_ptr + bitmap->pitch + sizeof(u32));
                    
                    v4 texel_a = {
                        (f32)((texel_ptr_a >> 16) & 0xff),
                        (f32)((texel_ptr_a >> 8)  & 0xff),
                        (f32)((texel_ptr_a >> 0)  & 0xff),
                        (f32)((texel_ptr_a >> 24) & 0xff)
                    };
                    
                    v4 texel_b = {
                        (f32)((texel_ptr_b >> 16) & 0xff),
                        (f32)((texel_ptr_b >> 8)  & 0xff),
                        (f32)((texel_ptr_b >> 0)  & 0xff),
                        (f32)((texel_ptr_b >> 24) & 0xff)
                    };
                    
                    v4 texel_c = {
                        (f32)((texel_ptr_c >> 16) & 0xff),
                        (f32)((texel_ptr_c >> 8)  & 0xff),
                        (f32)((texel_ptr_c >> 0)  & 0xff),
                        (f32)((texel_ptr_c >> 24) & 0xff)
                    };
                    
                    v4 texel_d = {
                        (f32)((texel_ptr_d >> 16) & 0xff),
                        (f32)((texel_ptr_d >> 8)  & 0xff),
                        (f32)((texel_ptr_d >> 0)  & 0xff),
                        (f32)((texel_ptr_d >> 24) & 0xff)
                    };
                    texel = lerp(lerp(texel_a, f_x, texel_b),
                                 f_y,
                                 lerp(texel_c, f_x, texel_d));
                }
                //alpha
                {
                    f32 src_r = texel.r;
                    f32 src_g = texel.g;
                    f32 src_b = texel.b;
                    f32 src_a = texel.a;
                    
                    f32 src_r_a = (src_a / 255.0f) * color.a;
                    
                    f32 dst_a = (f32)((*pixel >> 24) & 0xff);
                    f32 dst_r = (f32)((*pixel >> 16) & 0xff);
                    f32 dst_g = (f32)((*pixel >> 8)  & 0xff);
                    f32 dst_b = (f32)((*pixel >> 0)  & 0xff);
                    f32 dst_r_a = (dst_a / 255.0f);
                    
                    // (1-t)A + tB 
                    // alpha (0 - 1);
                    // color = dst_color + alpha (src_color - dst_color)
                    // color = (1 - alpha)dst_color + alpha * src_color;
                    
                    // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
                    
                    // linear blending
                    f32 inv_r_src_alpha = 1.0f - src_r_a;
                    f32 a = 255.0f * (src_r_a + dst_r_a - src_r_a * dst_r_a);
                    f32 r = inv_r_src_alpha * dst_r + src_r;
                    f32 g = inv_r_src_alpha * dst_g + src_g;
                    f32 b = inv_r_src_alpha * dst_b + src_b;
                    
                    *pixel = (((u32)(a + 0.5f) << 24)|
                              ((u32)(r + 0.5f) << 16)|
                              ((u32)(g + 0.5f) << 8 )|
                              ((u32)(b + 0.5f) << 0 ));
                    
                }
                
                //*pixel = texel;
            }
            pixel++;
        }
        row += buffer->pitch;
    }
}

inline
void
draw_rect_outline(Loaded_bmp* buffer, v2 start, v2 end, v4 color, f32 thickness = 2.0f) {
    
    draw_rect(buffer, v2{start.x - thickness, start.y - thickness}, v2{end.x   + thickness, start.y + thickness}, color);
    draw_rect(buffer, v2{start.x - thickness, end.y   - thickness}, v2{end.x   + thickness, end.y   + thickness}, color);
    
    draw_rect(buffer, v2{start.x - thickness, start.y - thickness}, v2{start.x + thickness, end.y + thickness}, color);
    draw_rect(buffer, v2{end.x   - thickness, start.y - thickness}, v2{end.x   + thickness, end.y + thickness}, color);
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
            f32 src_a = (f32)((*source >> 24) & 0xff);
            f32 src_r_a = (src_a / 255.0f) * c_alpha;
            
            f32 src_r = c_alpha * (f32)((*source >> 16) & 0xff);
            f32 src_g = c_alpha * (f32)((*source >> 8)  & 0xff);
            f32 src_b = c_alpha * (f32)((*source >> 0)  & 0xff);
            
            f32 dst_a = (f32)((*dest >> 24) & 0xff);
            f32 dst_r = (f32)((*dest >> 16) & 0xff);
            f32 dst_g = (f32)((*dest >> 8)  & 0xff);
            f32 dst_b = (f32)((*dest >> 0)  & 0xff);
            f32 dst_r_a = (dst_a / 255.0f);
            
            // (1-t)A + tB 
            // alpha (0 - 1);
            // color = dst_color + alpha (src_color - dst_color)
            // color = (1 - alpha)dst_color + alpha * src_color;
            
            // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
            
            // linear blending
            f32 inv_r_src_alpha = 1.0f - src_r_a;
            f32 a = 255.0f * (src_r_a + dst_r_a - src_r_a * dst_r_a);
            f32 r = inv_r_src_alpha * dst_r + src_r;
            f32 g = inv_r_src_alpha * dst_g + src_g;
            f32 b = inv_r_src_alpha * dst_b + src_b;
            
            *dest = (((u32)(a + 0.5f) << 24)|
                     ((u32)(r + 0.5f) << 16)|
                     ((u32)(g + 0.5f) << 8 )|
                     ((u32)(b + 0.5f) << 0 ));
            
            dest++;
            source++;
        }
        
        dest_row += bitmap_buffer->pitch;
        source_row += bitmap->pitch;
    }
}

inline
v2
get_render_entit_basis_pos(Render_group* render_group, Render_entity_basis* basis, v2 screen_center) {
    
    v3 entity_base_point = basis->basis->position;
    f32 z_fudge = 1.0f + 0.1f * (entity_base_point.z + basis->offset_z);
    
    f32 meters_to_pixels = render_group->meters_to_pixels;
    
    v2 entity_ground_point = { 
        screen_center.x + entity_base_point.x * z_fudge * meters_to_pixels,
        screen_center.y - entity_base_point.y * z_fudge * meters_to_pixels
    };
    f32 entity_z = -meters_to_pixels * entity_base_point.z;
    
    v2 center = { 
        entity_ground_point.x + basis->offset.x, 
        entity_ground_point.y + basis->offset.y + basis->entity_zc * entity_z
    };
    
    return center;
}

internal
void
render_group_to_output(Render_group* render_group, Loaded_bmp* output_target) {
    v2 screen_center = { 
        (f32)output_target->width  * 0.5f,
        (f32)output_target->height * 0.5f
    };
    
    for (u32 base_addr = 0; base_addr < render_group->push_buffer_size;) {
        
        Render_group_entry_header* header = (Render_group_entry_header*)(render_group->push_buffer_base + base_addr);
        
        switch (header->type) {
            case Render_group_entry_type_Render_entry_clear: {
                Render_entry_clear* entry = (Render_entry_clear*)header;
                base_addr += sizeof(*entry);
                
                v2 end = {
                    (f32)output_target->width,
                    (f32)output_target->height
                };
                draw_rect(output_target, v2{0,0}, end, entry->color);
                
            } break;
            
            case Render_group_entry_type_Render_entry_bitmap: {
                Render_entry_bitmap* entry = (Render_entry_bitmap*)header;
                base_addr += sizeof(*entry);
                
                v2 pos = get_render_entit_basis_pos(render_group, &entry->entity_basis, screen_center);
                
                macro_assert(entry->bitmap);
                
                draw_bitmap(output_target, entry->bitmap, pos, entry->color.a);
                
            } break;
            
            case Render_group_entry_type_Render_entry_rect: {
                Render_entry_rect* entry = (Render_entry_rect*)header;
                base_addr += sizeof(*entry);
                
                v2 pos = get_render_entit_basis_pos(render_group, &entry->entity_basis, screen_center);
                
                draw_rect(output_target, pos, pos + entry->dim, entry->color);
            } break;
            
            case Render_group_entry_type_Render_entry_coord_system: {
                Render_entry_coord_system* entry = (Render_entry_coord_system*)header;
                base_addr += sizeof(*entry);
                
                v2 pos = entry->origin;
                v2 dim = v2{2,2};
                
                v2 end = entry->origin + entry->x_axis + entry->y_axis;
                draw_rect_slow(output_target, entry->origin, entry->x_axis, entry->y_axis, entry->color, entry->bitmap);
                
                draw_rect(output_target, pos - dim, pos + dim, entry->color);
                
                pos = entry->origin + entry->x_axis;
                draw_rect(output_target, pos - dim, pos + dim, entry->color);
                
                pos = entry->origin + entry->y_axis;
                draw_rect(output_target, pos - dim, pos + dim, entry->color);
                
                draw_rect(output_target, end - dim, end + dim, entry->color);
#if 0
                for (u32 i = 0; i < macro_array_count(entry->points); i++) {
                    v2 p = entry->points[i];
                    p = entry->origin + p.x * entry->x_axis + p.y * entry->y_axis;
                    draw_rect(output_target, p - dim, p + dim, entry->color);
                }
#endif
            } break;
            
            default: {
                macro_assert(!"CRASH");
            } break;
        }
    }
    
}

internal
Render_group*
allocate_render_group(Memory_arena* arena, u32 max_push_buffer_size, f32 meters_to_pixels) {
    Render_group* result = mem_push_struct(arena, Render_group);
    result->push_buffer_base = (u8*)mem_push_size(arena, max_push_buffer_size);
    
    result->default_basis = mem_push_struct(arena, Render_basis);
    result->default_basis->position = v3{0,0,0};
    result->meters_to_pixels = meters_to_pixels;
    
    result->max_push_buffer_size = max_push_buffer_size;
    result->push_buffer_size = 0;
    
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
Render_entry_coord_system*
push_coord_system(Render_group* group, v2 origin, v2 x_axis, v2 y_axis, v4 color, Loaded_bmp* bitmap) {
    Render_entry_coord_system* entry = push_render_element(group, Render_entry_coord_system);
    
    if (!entry)
        return entry;
    
    entry->origin = origin;
    entry->x_axis = x_axis;
    entry->y_axis = y_axis;
    entry->color = color;
    entry->bitmap = bitmap;
    
    return entry;
}