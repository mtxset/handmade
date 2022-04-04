#include "main.h"

inline
void*
push_render_element(Render_group* group, u32 size) {
    void* result = 0;
    
    if ((group->push_buffer_size + size) < group->max_push_buffer_size) {
        result = (group->push_buffer_base + group->push_buffer_size);
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
    
    Entity_visible_piece* piece = (Entity_visible_piece*)push_render_element(group, sizeof(Entity_visible_piece));
    
    piece->basis     = group->default_basis;
    piece->bitmap    = bitmap;
    piece->offset    = group->meters_to_pixels * v2 { offset.x, -offset.y } - align;
    piece->offset_z  = offset_z;
    piece->entity_zc = entity_zc;
    piece->color     = color;
    piece->dim       = dim;
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
    push_piece(group, 0, offset, offset_z, v2{0, 0}, dim, color, entity_zc);
}

inline
void
push_rect_outline(Render_group* group, v2 offset, f32 offset_z, v2 dim, v4 color, f32 entity_zc = 1.0f) {
    
    f32 thickness = 0.1f;
    push_piece(group, 0, offset - v2{0, dim.y/2}, offset_z, v2{0, 0}, v2{dim.x, thickness}, color, entity_zc);
    push_piece(group, 0, offset + v2{0, dim.y/2}, offset_z, v2{0, 0}, v2{dim.x, thickness}, color, entity_zc);
    
    push_piece(group, 0, offset - v2{dim.x/2, 0}, offset_z, v2{0, 0}, v2{thickness, dim.y}, color, entity_zc);
    push_piece(group, 0, offset + v2{dim.x/2, 0}, offset_z, v2{0, 0}, v2{thickness, dim.y}, color, entity_zc);
}


internal
Render_group*
allocate_render_group(Memory_arena* arena, u32 max_push_buffer_size, f32 meters_to_pixels) {
    Render_group* result = mem_push_struct(arena, Render_group);
    result->push_buffer_base = (u8*)mem_push_size(arena, max_push_buffer_size);
    
    result->default_basis = mem_push_struct(arena, Render_basis);
    result->default_basis->position = v3{0,0,0};
    result->meters_to_pixels = meters_to_pixels;
    result->count = 0;
    
    result->max_push_buffer_size = max_push_buffer_size;
    result->push_buffer_size = 0;
    
    return result;
}