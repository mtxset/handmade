/* date = April 1st 2022 1:31 pm */

#ifndef RENDER_GROUP_H
#define RENDER_GROUP_H

struct Render_basis {
    v3 position;
};

struct Entity_visible_piece {
    Render_basis* basis;
    Loaded_bmp* bitmap;
    v2 offset;
    f32 offset_z;
    f32 entity_zc;
    v4 color;
    v2 dim;
};

struct Render_group {
    Render_basis* default_basis;
    f32 meters_to_pixels;
    u32 count;
    
    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8* push_buffer_base;
};

#endif //RENDER_GROUP_H
