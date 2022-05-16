/* date = April 1st 2022 1:31 pm */

#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

struct Loaded_bmp {
    v2 align;
    i32 height;
    i32 width;
    i32 pitch;
    void* memory;
};

struct Env_map {
    Loaded_bmp lod[4];
    f32 pos_z;
};

struct Render_basis {
    v3 position;
};

struct Render_entity_basis {
    Render_basis* basis;
    v3 offset;
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
    Render_entity_basis entity_basis;
    v2 dim;
    v4 color;
};

struct Render_entry_bitmap {
    Render_entity_basis entity_basis;
    Loaded_bmp* bitmap;
    
    v4 color;
};

struct Render_group {
    Render_basis* default_basis;
    f32 meters_to_pixels;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8* push_buffer_base;
};

#endif //RENDER_GROUP_H
