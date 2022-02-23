#include <stdio.h>

#include "types.h"
#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "intrinsics.h"

#include "game.h"
#include "world.h"
#include "world.cpp"
#include "random.h"
#include "color.h"
#include "test.cpp"
#include "sim_region.cpp"
#include "entity.cpp"

#if 0
#define RUN_RAY
#endif

internal
void 
clear_screen(Loaded_bmp* bitmap_buffer, u32 color) {
    // 8 bit pointer to the beginning of the memory
    // 8 bit so we have arithemtic by 1 byte
    auto row = (u8*)bitmap_buffer->memory;
    for (i32 y = 0; y < bitmap_buffer->height; y++) {
        // 32 bits so we move by 4 bytes cuz aa rr gg bb
        auto pixel = (u32*)row;
        for (i32 x = 0; x < bitmap_buffer->width; x++) {
            *pixel++ = color;
        }
        row += bitmap_buffer->pitch;
    }
}

internal
void
render_255_gradient(Loaded_bmp* bitmap_buffer, i32 blue_offset, i32 green_offset) {
    local_persist i32 offset = 1;
    auto row = (u8*)bitmap_buffer->memory;
    
    clear_screen(bitmap_buffer, color_gray_byte);
    
    for (i32 y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (u32*)row;
        for (i32 x = 0; x < bitmap_buffer->width; x++) {
            // pixel bytes	   1  2  3  4
            // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
            // little endian
            
            u8 blue = (u8)(x + blue_offset + offset);
            u8 green = (u8)(y + green_offset);
            
            // 0x 00 00 00 00 -> 0x xx rr gg bb
            // | composites bytes
            // green << 8 - shifts by 8 bits
            // other stay 00
            // * dereference pixel
            // pixel++ - pointer arithmetic - jumps by it's size (32 bits in this case)
            *pixel++ = (green << 8) | blue;
            
        }
        row += bitmap_buffer->pitch;
    }
    
    local_persist bool go_up = true;
    i32 step = 1;
    
    if (go_up)
        offset += step;
    else
        offset -= step;
    
    if (offset >= 100)
        go_up = false;
    
    if (offset <= 0)
        go_up = true;
}

internal
void 
game_output_sound(Game_sound_buffer* sound_buffer, i32 tone_hz, Game_state* state) {
    i16 tone_volume = 3000;
    i32 wave_period = sound_buffer->samples_per_second / tone_hz;
    auto sample_out = sound_buffer->samples;
    
#if 1
    for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        i16 sample_value = 0;
        
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
    }
#else
    for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        f32 sine_val = sinf(state->t_sine);
        i16 sample_value = (i16)(sine_val * tone_volume);
        
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        state->t_sine += PI * 2.0f * (1.0f / (f32)wave_period);
        
        // cuz sinf loses its floating point precision???
        if (state->t_sine > PI * 2.0f)
            state->t_sine -= PI * 2.0f;
    }
#endif
}

internal
void 
draw_rect(Loaded_bmp* buffer, v2 start, v2 end, v3 color) {
    u32 color_hex = 0;
    
    color_hex = (round_f32_u32(color.r * 255.0f) << 16 | 
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

inline
void 
draw_rect_outline(Loaded_bmp* buffer, v2 start, v2 end, v3 color, f32 thickness = 2.0f) {
    
    draw_rect(buffer, v2{start.x - thickness, start.y - thickness}, v2{end.x   + thickness, start.y + thickness}, color);
    draw_rect(buffer, v2{start.x - thickness, end.y   - thickness}, v2{end.x   + thickness, end.y   + thickness}, color);
    
    draw_rect(buffer, v2{start.x - thickness, start.y - thickness}, v2{start.x + thickness, end.y + thickness}, color);
    draw_rect(buffer, v2{end.x   - thickness, start.y - thickness}, v2{end.x   + thickness, end.y + thickness}, color);
}

internal
void 
debug_read_and_write_random_file() {
    auto bitmap_read = debug_read_entire_file(__FILE__);
    if (bitmap_read.content) {
        debug_write_entire_file("temp.cpp", bitmap_read.bytes_read, bitmap_read.content);
        debug_free_file(bitmap_read.content);
    }
}

internal
void 
draw_pixel(Loaded_bmp* bitmap_buffer, v2 pos, v3 color) {
    v2 end = { pos.x, pos.y };
    
    v2 screen_center = { 
        (f32)bitmap_buffer->width / 2,
        (f32)bitmap_buffer->height / 2 
    };
    
    pos.y = -pos.y;
    end.y = -end.y;
    
    end = { pos.x + 1.0f, pos.y + 1.0f };
    
    pos += screen_center;
    end += screen_center;
    
    draw_rect(bitmap_buffer, pos, end, color);
}

internal
void 
draw_circle(Loaded_bmp* bitmap_buffer, v2 start, f32 radius, v3 color) {
    // y = sin (angle) * r
    // x = cos (angle) * r
    for (f32 angle = 0; angle < 360; angle++) {
        v2 v = { 
            cos(angle) * radius, 
            sin(angle) * radius 
        };
        draw_pixel(bitmap_buffer, v + start, color);
    }
}

internal
void 
draw_line(Loaded_bmp* bitmap_buffer, v2 start, v2 end, v3 color, u32 points = 100) {
    f32 m = (end.y - start.y) / (end.x - start.x);
    // m - slope
    // b - intercept
    
    // y - y1 = m(x - x1)
    // y = m(x - x1) + y1
    v2 vector = end - start;
    
    f32 x, i, inc;
    f32 h = square_root(square(vector.x) + square(vector.y));
    
    inc = vector.x / points;
    
    if (end.x > start.x) {
        for (x = start.x, i = 0; i <= points; x += inc, i++) {
            f32 y = m * (x - start.x) + start.y;
            v2 pixel = { x, y };
            draw_pixel(bitmap_buffer, pixel, color);
        }
    }
    else {
        for (x = end.x, i = 0; i <= points; x -= inc, i++) {
            f32 y = m * (x - start.x) + start.y;
            v2 pixel = { x, y };
            draw_pixel(bitmap_buffer, pixel, color);
        }
    }
}

internal
void 
vectors_update(Loaded_bmp* bitmap_buffer, Game_state* state, Game_input* input) {
    clear_screen(bitmap_buffer, color_black_byte);
    
    f32 x = 100;
    f32 y = 100;
    
    v2 red_vec   = { x, y };
    v2 green_vec = { -x, y };
    v2 blue_vec  = { -x, -y };
    v2 white_vec = { x, -y };
    
    v2 vec = white_vec;
    
    v2 screen_center = { 
        (f32)bitmap_buffer->width / 2,
        (f32)bitmap_buffer->height / 2 
    };
    
    draw_pixel(bitmap_buffer, { 0, 0 }, gold_v3);
    draw_pixel(bitmap_buffer, { 0, x }, red_v3);
    draw_pixel(bitmap_buffer, { x, 0 }, red_v3);
    draw_pixel(bitmap_buffer, { 0, -x }, red_v3);
    draw_pixel(bitmap_buffer, { -x, 0 }, red_v3);
    
    draw_line(bitmap_buffer, { 0, 0 }, vec, green_v3);
    draw_line(bitmap_buffer, { 0, 0 }, perpendicular_v2(vec), green_v3);
}

internal
void 
each_pixel(Loaded_bmp* bitmap_buffer, Game_state* state, f32 time_delta) {
    clear_screen(bitmap_buffer, color_black_byte);
    
    Each_monitor_pixel* pixel_state = &state->monitor_pixels;
    
    if ((pixel_state->timer += time_delta) > 0.01f) {
        pixel_state->timer = 0;
        pixel_state->x++;
        
        if (pixel_state->x > bitmap_buffer->width) {
            pixel_state->x = 0;
            pixel_state->y++;
        }
        
        if (pixel_state->y > bitmap_buffer->height) {
            pixel_state->x = 0;
            pixel_state->y = 0;
        }
    }
    draw_pixel(bitmap_buffer, v2 { pixel_state->x, pixel_state->y }, red_v3);
}

internal
void 
drops_update(Loaded_bmp* bitmap_buffer, Game_state* state, Game_input* input) {
    
    // draw mouse pointer
    f32 mouse_draw_offset = 10.0f; // offset from windows layer
    v3 color_white = { 1.0f, 1.0f, 1.0f };
    {
        v2 mouse_start = { 
            (f32)input->mouse_x - mouse_draw_offset,
            (f32)input->mouse_y - mouse_draw_offset 
        };
        
        v2 mouse_end = {
            (f32)input->mouse_x,
            (f32)input->mouse_y
        };
        
        draw_rect(bitmap_buffer, mouse_start, mouse_end, color_white);
        
        if (input->mouse_buttons[0].ended_down) {
            i32 index = state->drop_index++;
            auto drop = &state->drops[index];
            drop->active = true;
            drop->a = 0;
            drop->pos = mouse_start;
            
            if (state->drop_index >= macro_array_count(state->drops))
                state->drop_index = 0;
        }
    }
    
    // move drops
    {
        for (int i = 0; i < macro_array_count(state->drops); i++) {
            auto drop = &state->drops[i];
            
            if (!drop->active)
                continue;
            
            drop->a += 0.1f;
            drop->pos.y += 3.0f * input->time_delta + drop->a;
            
            if (drop->pos.y >= bitmap_buffer->height) {
                drop->active = false;
            }
        }
    }
    
    // draw drops
    {
        for (int i = 0; i < macro_array_count(state->drops); i++) {
            auto drop = &state->drops[i];
            
            if (!drop->active)
                continue;
            
            v2 drop_end = {
                drop->pos.x + mouse_draw_offset, 
                drop->pos.y + mouse_draw_offset
            };
            
            draw_rect(bitmap_buffer, drop->pos, drop_end , color_white);
        }
    }
}

#if 0
#include "pacman.cpp"
#endif

internal
Loaded_bmp 
debug_load_bmp(char* file_name) {
    Loaded_bmp result = {};
    
    Debug_file_read_result file_result = debug_read_entire_file(file_name);
    
    macro_assert(file_result.bytes_read != 0);
    
    Bitmap_header* header = (Bitmap_header*)file_result.content;
    
    macro_assert(header->Compression == 3);
    
    result.memory = (u32*)((u8*)file_result.content + header->BitmapOffset);
    
    result.width  = header->Width;
    result.height = header->Height;
    
    u32 mask_red   = header->RedMask;
    u32 mask_green = header->GreenMask;
    u32 mask_blue  = header->BlueMask;
    u32 mask_alpha = ~(mask_red | mask_green | mask_blue);
    
    Bit_scan_result scan_red   = find_least_significant_first_bit(mask_red);
    Bit_scan_result scan_green = find_least_significant_first_bit(mask_green);
    Bit_scan_result scan_blue  = find_least_significant_first_bit(mask_blue);
    Bit_scan_result scan_alpha = find_least_significant_first_bit(mask_alpha);
    
    macro_assert(scan_red.found);
    macro_assert(scan_green.found);
    macro_assert(scan_blue.found);
    macro_assert(scan_alpha.found);
    
    i32 shift_alpha_down = (i32)scan_alpha.index;
    i32 shift_red_down   = (i32)scan_red.index;
    i32 shift_green_down = (i32)scan_green.index;
    i32 shift_blue_down  = (i32)scan_blue.index;
    
    u32* source_dest = (u32*)result.memory;
    for (i32 y = 0; y < header->Height; y++) {
        for (i32 x = 0; x < header->Width; x++) {
            u32 color = *source_dest;
            
            f32 r = (f32)((color & mask_red)   >> shift_red_down);
            f32 g = (f32)((color & mask_green) >> shift_green_down);
            f32 b = (f32)((color & mask_blue)  >> shift_blue_down);
            f32 a = (f32)((color & mask_alpha) >> shift_alpha_down);
            
            f32 a_norm = a / 255.0f;
            
            r = r * a_norm;
            g = g * a_norm;
            b = b * a_norm;
            
            *source_dest++ = (((u32)(a + 0.5f) << 24) | 
                              ((u32)(r + 0.5f) << 16) | 
                              ((u32)(g + 0.5f) << 8 ) | 
                              ((u32)(b + 0.5f) << 0 ));
        }
    }
    
    i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
    result.pitch = -result.width * bytes_per_pixel;
    result.memory = (u8*)result.memory - result.pitch * (result.height - 1);
    
    return result;
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
            
            *dest = (((u32)(a + 0.5f) << 24) | 
                     ((u32)(r + 0.5f) << 16) | 
                     ((u32)(g + 0.5f) << 8 ) | 
                     ((u32)(b + 0.5f) << 0 ));
            
            dest++;
            source++;
        }
        
        dest_row += bitmap_buffer->pitch;
        source_row += bitmap->pitch;
    }
}

internal
void 
subpixel_test_update(Loaded_bmp* buffer, Game_state* game_state, Game_input* input, v3 color) {
    
    clear_screen(buffer, color_black_byte);
    
    i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
    u8* row = (u8*)buffer->memory + (buffer->width / 2) * bytes_per_pixel + (buffer->height / 2) * buffer->pitch;
    u32* pixel_one = (u32*)row;
    u32* pixel_two = pixel_one + 1;
    
    Subpixel_test* subpixels = &game_state->subpixels;
    
    if ((subpixels->pixel_timer += input->time_delta) >= 0.01f) {
        subpixels->pixel_timer = 0;
        
        if (subpixels->direction)
            subpixels->transition_state -= 0.01f;
        else
            subpixels->transition_state += 0.01f;
        
        if (subpixels->transition_state > 1.0f || subpixels->transition_state < 0.0f) {
            subpixels->transition_state = clamp_f32(subpixels->transition_state, 0.0f, 1.0f);
            
            subpixels->direction = !subpixels->direction;
        }
    }
    
    u32 color_hex = (round_f32_u32(color.r * 255.0f * subpixels->transition_state) << 16 | 
                     round_f32_u32(color.g * 255.0f * subpixels->transition_state) << 8  |
                     round_f32_u32(color.b * 255.0f * subpixels->transition_state) << 0);
    
    *pixel_one = color_hex;
    
    f32 start_y = 400;
    f32 end_y = start_y + 100;
    
    draw_rect(buffer, v2 { 300.0f, start_y } , v2 { 400.0f, end_y }, 
              {
                  color.r * subpixels->transition_state, 
                  color.g * subpixels->transition_state, 
                  color.b * subpixels->transition_state });
    
    color_hex = (round_f32_u32(color.r * 255.0f * (1 - subpixels->transition_state)) << 16 | 
                 round_f32_u32(color.g * 255.0f * (1 - subpixels->transition_state)) << 8  |
                 round_f32_u32(color.b * 255.0f * (1 - subpixels->transition_state)) << 0);
    
    draw_rect(buffer, 
              { 400.0f, start_y }, { 500.0f, end_y }, 
              { color.r * (1 - subpixels->transition_state),
                  color.g * (1 - subpixels->transition_state), 
                  color.b * (1 - subpixels->transition_state) });
    
    *pixel_two = color_hex;
}

internal
Add_low_entity_result 
add_low_entity(Game_state* game_state, Entity_type type, World_position pos) {
    macro_assert(game_state->low_entity_count < macro_array_count(game_state->low_entity_list));
    
    Add_low_entity_result result;
    u32 entity_index = game_state->low_entity_count++;
    
    Low_entity* entity_low = game_state->low_entity_list + entity_index;
    *entity_low = {};
    entity_low->sim.type = type;
    entity_low->sim.collision = game_state->null_collision;
    entity_low->position = null_position();
    
    change_entity_location(&game_state->world_arena, game_state->world, entity_index, entity_low, pos);
    
    result.low = entity_low;
    result.low_index = entity_index;
    
    return result;
}

internal
void 
init_hit_points(Low_entity* entity_low, u32 hit_point_count) {
    macro_assert(hit_point_count <= macro_array_count(entity_low->sim.hit_point));
    entity_low->sim.hit_points_max = hit_point_count;
    
    for (u32 hit_point_index = 0; hit_point_index < entity_low->sim.hit_points_max; hit_point_index++) {
        Hit_point* hit_point = entity_low->sim.hit_point + hit_point_index;
        hit_point->flags = 0;
        hit_point->filled_amount = HIT_POINT_SUB_COUNT;
    }
}

internal 
Add_low_entity_result
add_grounded_entity(Game_state* game_state, Entity_type type, World_position pos, Sim_entity_collision_group* collision)
{
    Add_low_entity_result entity = add_low_entity(game_state, type, pos);
    entity.low->sim.collision = collision;
    
    return entity;
}

internal
World_position 
chunk_pos_from_tile_pos(World* world, i32 abs_tile_x, i32 abs_tile_y, i32 abs_tile_z, v3 additional_offset = v3{0,0,0}) {
    
    World_position result = {};
    World_position base_pos = {};
    
    f32 tile_side_meters = 1.4f;
    f32 tile_depth_meters = 3.0f;
    
    v3 tile_side_dim = v3 { tile_side_meters, tile_side_meters, tile_depth_meters };
    v3 abs_tile_dim = v3 {(f32)abs_tile_x, (f32)abs_tile_y, (f32)abs_tile_z};
    v3 offset = hadamard(tile_side_dim, abs_tile_dim);
    
    result = map_into_chunk_space(world, base_pos, offset + additional_offset);
    macro_assert(is_canonical(world, result._offset));
    
    return result;
}

internal
Add_low_entity_result
add_std_room(Game_state* game_state, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    
    World_position pos = chunk_pos_from_tile_pos(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_space, pos, game_state->std_room_collision);
    
    add_flags(&entity.low->sim, Entity_flag_traversable);
    
    return entity;
}

internal
Add_low_entity_result 
add_sword(Game_state* game_state) {
    Add_low_entity_result entity = add_low_entity(game_state, Entity_type_sword, null_position());
    
    add_flags(&entity.low->sim, Entity_flag_moveable);
    entity.low->sim.collision = game_state->sword_collision;
    
    return entity;
}

internal
Add_low_entity_result 
add_player(Game_state* game_state) {
    
    World_position pos = game_state->camera_pos;
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_hero, pos, game_state->player_collision);
    
    init_hit_points(entity.low, 3);
    
    add_flags(&entity.low->sim, Entity_flag_collides|Entity_flag_moveable);
    
    Add_low_entity_result sword = add_sword(game_state);
    entity.low->sim.sword.index = sword.low_index;
    
    if (game_state->following_entity_index == 0) {
        game_state->following_entity_index = entity.low_index;
    }
    
    return entity;
}

internal
Add_low_entity_result 
add_monster(Game_state* game_state, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    
    World_position pos = chunk_pos_from_tile_pos(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_monster, pos, game_state->monster_collision);
    
    init_hit_points(entity.low, 2);
    
    add_flags(&entity.low->sim, Entity_flag_collides|Entity_flag_moveable);
    
    return entity;
}

internal
Add_low_entity_result 
add_familiar(Game_state* game_state, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    
    World_position pos = chunk_pos_from_tile_pos(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_familiar, pos, game_state->familiar_collision);
    
    add_flags(&entity.low->sim, Entity_flag_collides|Entity_flag_moveable);
    
    return entity;
}

internal
Add_low_entity_result 
add_wall(Game_state* game_state, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    
    World_position pos = chunk_pos_from_tile_pos(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_wall, pos, game_state->wall_collision);
    
    add_flags(&entity.low->sim, Entity_flag_collides);
    
    return entity;
}

internal
Add_low_entity_result 
add_stairs(Game_state* game_state, u32 abs_tile_x, u32 abs_tile_y, u32 abs_tile_z) {
    
    World_position pos = chunk_pos_from_tile_pos(game_state->world, abs_tile_x, abs_tile_y, abs_tile_z);
    Add_low_entity_result entity = add_grounded_entity(game_state, Entity_type_stairwell, pos, game_state->stairs_collision);
    
    add_flags(&entity.low->sim, Entity_flag_collides);
    entity.low->sim.walkable_dim = entity.low->sim.collision->total_volume.dim.xy;
    entity.low->sim.walkable_height = game_state->typical_floor_height;
    
    return entity;
}


#ifdef RUN_RAY
#include "ray.h"
extern "C"
void game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
    local_persist u32 counter = 120;
    if (counter++ >= 120) {
        clear_screen(bitmap_buffer, color_gray_byte);
        counter = 0;
        ray_udpate(bitmap_buffer);
    }
}
#else

inline
void 
push_piece(Entity_visible_piece_group* group, Loaded_bmp* bitmap, 
           v2 offset, f32 offset_z, v2 align, v2 dim, v4 color, f32 entity_zc) {
    macro_assert(group->count < macro_array_count(group->piece_list));
    Entity_visible_piece* piece = group->piece_list + group->count++;
    
    piece->bitmap    = bitmap;
    piece->offset    = group->game_state->meters_to_pixels * v2 { offset.x, -offset.y } - align;
    piece->offset_z  = offset_z;
    piece->entity_zc = entity_zc;
    piece->color     = color;
    piece->dim       = dim;
}

inline
void 
push_bitmap(Entity_visible_piece_group* group, Loaded_bmp* bitmap, 
            v2 offset, f32 offset_z, v2 align, f32 alpha = 1.0f, f32 entity_zc = 1.0f) {
    push_piece(group, bitmap, offset, offset_z, align, v2 {0, 0}, v4 {1.0f, 1.0f, 1.0f, alpha }, entity_zc);
}

inline
void 
push_rect(Entity_visible_piece_group* group, v2 offset, f32 offset_z, v2 dim, v4 color, f32 entity_zc = 1.0f) {
    push_piece(group, 0, offset, offset_z, v2{0, 0}, dim, color, entity_zc);
}

inline
void 
push_rect_outline(Entity_visible_piece_group* group, v2 offset, f32 offset_z, v2 dim, v4 color, f32 entity_zc = 1.0f) {
    
    f32 thickness = 0.1f;
    push_piece(group, 0, offset - v2{0, dim.y/2}, offset_z, v2{0, 0}, v2{dim.x, thickness}, color, entity_zc);
    push_piece(group, 0, offset + v2{0, dim.y/2}, offset_z, v2{0, 0}, v2{dim.x, thickness}, color, entity_zc);
    
    push_piece(group, 0, offset - v2{dim.x/2, 0}, offset_z, v2{0, 0}, v2{thickness, dim.y}, color, entity_zc);
    push_piece(group, 0, offset + v2{dim.x/2, 0}, offset_z, v2{0, 0}, v2{thickness, dim.y}, color, entity_zc);
}

inline
void
draw_hitpoints(Entity_visible_piece_group* piece_group, Sim_entity* entity) {
    if (entity->hit_points_max > 0) {
        v2 health_dim = { 0.2f, 0.2f };
        f32 spacing_x = 1.5f * health_dim.x;
        
        v2 hit_p = { 
            -0.5f * (entity->hit_points_max - 1) * spacing_x, 
            -0.25f 
        };
        
        v2 d_hit_p = {
            spacing_x,
            0.0f
        };
        
        for (u32 health_index = 0; health_index < entity->hit_points_max; health_index++) {
            Hit_point* hit_point = entity->hit_point + health_index;
            
            v4 color = { 1.0f, 0.0f, 0.0f, 1.0f };
            
            if (hit_point->filled_amount == 0) {
                color = { 0.2f, 0.2f, 0.2f, 1.0f };
            }
            
            push_rect(piece_group, hit_p, 0, health_dim, color);
            hit_p += d_hit_p;
        }
    }
}

Sim_entity_collision_group*
make_null_collision(Game_state* game_state) {
    Sim_entity_collision_group* group = mem_push_struct(&game_state->world_arena, Sim_entity_collision_group);
    group->volume_count = 0;
    group->volume_list = 0;
    group->total_volume.offset_pos = v3 {0, 0, 0};
    group->total_volume.dim = v3 {0, 0, 0};
    
    return group;
}

Sim_entity_collision_group*
make_simple_grounded_collision(Game_state* game_state, f32 x, f32 y, f32 z) {
    Sim_entity_collision_group* group = mem_push_struct(&game_state->world_arena, Sim_entity_collision_group);
    group->volume_count = 1;
    group->volume_list = mem_push_array(&game_state->world_arena, group->volume_count, Sim_entity_collision_volume);
    group->total_volume.offset_pos = v3 {0, 0, 0.5f * z};
    group->total_volume.dim = v3 {x, y, z};
    group->volume_list[0] = group->total_volume;
    
    return group;
}

Sim_entity_collision_group*
make_simple_grounded_collision(Game_state* game_state, v3 dim) {
    Sim_entity_collision_group* group = make_simple_grounded_collision(game_state, dim.x, dim.y, dim.z);
    return group;
}

internal
void
fill_ground_chunk(Transient_state* tran_state, Game_state* game_state, Ground_buffer* ground_buffer, World_position* chunk_pos) {
    
    Random_series series = random_seed(139*chunk_pos->chunk_x + 593*chunk_pos->chunk_y + 329*chunk_pos->chunk_z);
    
    u32 random_number_index = 0;
    
    //v2 center = 0.5f * v2_i32(bitmap_buffer->width, bitmap_buffer->height);
    
    Loaded_bmp bitmap_buffer = tran_state->ground_buffer_template;
    bitmap_buffer.memory = ground_buffer->memory;
    
    ground_buffer->position = *chunk_pos;
    
    f32 width = (f32)bitmap_buffer.width;
    f32 height = (f32)bitmap_buffer.height;
    
    for (u32 grass_index = 0; grass_index < 100; grass_index++) {
        macro_assert(random_number_index < macro_array_count(random_number_table));
        
        Loaded_bmp* stamp;
        
        if (random_choise(&series, 2)) {
            u32 count = macro_array_count(game_state->grass);
            u32 random_index = random_choise(&series, count);
            
            stamp = game_state->grass + random_index;
        }
        else {
            u32 count = macro_array_count(game_state->stone);
            u32 random_index = random_choise(&series, count);
            stamp = game_state->stone + random_index;
        }
        
        v2 bitmap_center = 0.5f * v2_i32(stamp->width, stamp->height);
        v2 offset = {
            width  * random_unilateral(&series),
            height * random_unilateral(&series)
        };
        
        v2 pos = offset - bitmap_center;
        draw_bitmap(&bitmap_buffer, stamp, pos);
    }
    
    for (u32 grass_index = 0; grass_index < 100; grass_index++) {
        macro_assert(random_number_index < macro_array_count(random_number_table));
        
        Loaded_bmp* stamp;
        
        u32 count = macro_array_count(game_state->tuft);
        u32 random_index = random_choise(&series, count);
        stamp = game_state->tuft + random_index;
        
        v2 bitmap_center = 0.5f * v2_i32(stamp->width, stamp->height);
        v2 offset = {
            width  * random_unilateral(&series),
            height * random_unilateral(&series)
        };
        
        v2 pos = offset - bitmap_center;
        draw_bitmap(&bitmap_buffer, stamp, pos);
    }
    
}

internal
void
clear_bitmap(Loaded_bmp* bitmap) {
    if (bitmap->memory) {
        u32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
        u32 total_bitmap_size = bitmap->width * bitmap->height * bytes_per_pixel;
        mem_zero_size_(total_bitmap_size, bitmap->memory);
    }
}

internal
Loaded_bmp
make_empty_bitmap(Memory_arena* arena, i32 width, i32 height, bool clear_to_zero = true) {
    Loaded_bmp result = {};
    
    u32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
    result.width  = width;
    result.height = height;
    result.pitch  = result.width * bytes_per_pixel;
    
    u32 total_bitmap_size = width * height * bytes_per_pixel;
    result.memory = mem_push_size_(arena, total_bitmap_size);
    
    if (clear_to_zero)
        mem_zero_size_(total_bitmap_size, result.memory);
    
    return result;
}

#if 0
internal
void
request_ground_buffer(World_position center, Rect3 bounds) {
    bounds = offset(bounds, center._offset);
    center._offset = v3{0,0,0};
    fill_ground_chunk(tran_state, game_state, tran_state->ground_buffer_list, &game_state->camera_pos);
}
#endif

extern "C"
void 
game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
    
#if 0
    run_tests();
#endif
    
    u32 ground_buffer_width  = 256;
    u32 ground_buffer_height = 256;
    
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    macro_assert(sizeof(Game_state) <= memory->permanent_storage_size);
    
    Game_state* game_state = (Game_state*)memory->permanent_storage;
    
    const f32 floor_height = 3.0f;
    // init game state
    if (!memory->is_initialized) {
        const u32 tiles_per_width  = 17;
        const u32 tiles_per_height = 9;
        
        // init memory arenas
        initialize_arena(&game_state->world_arena,
                         memory->permanent_storage_size - sizeof(Game_state),
                         (u8*)memory->permanent_storage + sizeof(Game_state));
        
        // reserve null entity for ?
        add_low_entity(game_state, Entity_type_null, null_position());
        
        game_state->world = mem_push_struct(&game_state->world_arena, World);
        World* world = game_state->world;
        
        game_state->typical_floor_height = 3.0f;
        game_state->meters_to_pixels = 42.0f;
        game_state->pixels_to_meters = 1.0f / game_state->meters_to_pixels;
        
        v3 world_chunk_dim_meters = {
            (f32)ground_buffer_width * game_state->pixels_to_meters,
            (f32)ground_buffer_height * game_state->pixels_to_meters,
            game_state->typical_floor_height
        };
        
        init_world(world, world_chunk_dim_meters);
        
        // collision entries
        {
            f32 tile_side_meters = 1.4f;
            f32 tile_depth_meters = game_state->typical_floor_height;
            
            game_state->null_collision = make_null_collision(game_state);
            
            v3 sword_dim = { 0.5f, 1.0f, 0.1f };
            game_state->sword_collision = make_simple_grounded_collision(game_state, sword_dim);
            
            v3 stair_dim = { 
                tile_side_meters,
                2.0f * tile_side_meters,
                1.1f * tile_depth_meters
            };
            game_state->stairs_collision = make_simple_grounded_collision(game_state, stair_dim);
            
            v3 player_dim = { 1.0f, 0.5f, 1.2f };
            game_state->player_collision = make_simple_grounded_collision(game_state, player_dim);
            
            v3 monster_dim = { 1.0f, 0.5f, 0.5f };
            game_state->monster_collision = make_simple_grounded_collision(game_state, monster_dim);
            
            v3 wall_dim = { 
                tile_side_meters,
                tile_side_meters,
                tile_depth_meters
            };
            game_state->wall_collision = make_simple_grounded_collision(game_state, wall_dim);
            
            v3 room_dim = { 
                tiles_per_width  * tile_side_meters,
                tiles_per_height * tile_side_meters,
                0.9f * tile_depth_meters
            };
            game_state->std_room_collision = make_simple_grounded_collision(game_state, room_dim);
            
            v3 familiar_dim = { 1.0f, 0.5f, 0.5f };
            game_state->familiar_collision = make_simple_grounded_collision(game_state, familiar_dim);
        }
        
#if 0
        // pacman init
        {
            game_state->pacman_state.player_tile_x = 1;
            game_state->pacman_state.player_tile_y = 1;
            
            game_state->pacman_state.ghost_tile_x = 10;
            game_state->pacman_state.ghost_tile_y = 10;
            
            game_state->pacman_state.ghost_direction_y = 1;
        }
#endif
        
        // load sprites
        {
            game_state->grass[0]   = debug_load_bmp("../data/grass00.bmp");
            game_state->grass[1]   = debug_load_bmp("../data/grass01.bmp");
            
            game_state->tuft[0]    = debug_load_bmp("../data/tuft00.bmp");
            game_state->tuft[1]    = debug_load_bmp("../data/tuft01.bmp");
            game_state->tuft[2]    = debug_load_bmp("../data/tuft02.bmp");
            
            game_state->stone[0]   = debug_load_bmp("../data/ground00.bmp");
            game_state->stone[1]   = debug_load_bmp("../data/ground01.bmp");
            game_state->stone[2]   = debug_load_bmp("../data/ground02.bmp");
            game_state->stone[3]   = debug_load_bmp("../data/ground03.bmp");
            
            game_state->background = debug_load_bmp("../data/bg_nebula.bmp");
            game_state->tree       = debug_load_bmp("../data/tree.bmp");
            game_state->monster    = debug_load_bmp("../data/george-front-0.bmp");
            game_state->familiar   = debug_load_bmp("../data/ship.bmp");
            game_state->sword      = debug_load_bmp("../data/sword.bmp");
            game_state->stairwell  = debug_load_bmp("../data/george-back-0.bmp");
#if 0
            // load george
            {
                game_state->hero_bitmaps[0].hero_body = debug_load_bmp("../data/george-right-0.bmp");
                game_state->hero_bitmaps[0].align = v2 { 24, 41 };
                
                game_state->hero_bitmaps[1].hero_body = debug_load_bmp("../data/george-back-0.bmp");
                game_state->hero_bitmaps[1].align = v2 { 24, 41 };
                
                game_state->hero_bitmaps[2].hero_body = debug_load_bmp("../data/george-left-0.bmp");
                game_state->hero_bitmaps[2].align = v2 { 24, 41 };
                
                game_state->hero_bitmaps[3].hero_body = debug_load_bmp("../data/george-front-0.bmp");
                game_state->hero_bitmaps[3].align = v2 { 24, 41 };
            }
#else
            // load blender guy
            {
                game_state->hero_bitmaps[0].hero_body = debug_load_bmp("../data/right.bmp");
                game_state->hero_bitmaps[0].align = v2 { 38, 119 };
                
                game_state->hero_bitmaps[1].hero_body = debug_load_bmp("../data/back.bmp");
                game_state->hero_bitmaps[1].align = v2 { 40, 112 };
                
                game_state->hero_bitmaps[2].hero_body = debug_load_bmp("../data/left.bmp");
                game_state->hero_bitmaps[2].align = v2 { 40, 119 };
                
                game_state->hero_bitmaps[3].hero_body = debug_load_bmp("../data/front.bmp");
                game_state->hero_bitmaps[3].align = v2 { 39, 115 };
            }
#endif
        }
        
        u32 screen_base_x = 0;
        u32 screen_base_y = 0;
        u32 screen_base_z = 0;
        
        u32 screen_x        = screen_base_x;
        u32 screen_y        = screen_base_y;
        u32 absolute_tile_z = screen_base_z;
        
        // 256 x 256 tile chunks 
        
        // world generation
        Random_series series = random_seed(1234);
        
        {
            u32 room_goes_horizontal = 1;
            u32 room_has_stairs = 2;
            
            bool door_north = false;
            bool door_east  = false;
            bool door_south = false;
            bool door_west  = false;
            bool door_up    = false;
            bool door_down  = false;
            
            const u32 max_screens = 2000;
            
            for (u32 screen_index = 0; screen_index < max_screens; screen_index++) {
                //u32 door_direction = random_choise(&series, (door_up || door_down) ? 2 : 3);
                
                u32 door_direction = random_choise(&series, 2);
                
                bool created_z_door = false;
                if (door_direction == room_has_stairs) {
                    created_z_door = true;
                    if (absolute_tile_z == screen_base_z)
                        door_up = true;
                    else
                        door_down = true;
                }
                else if (door_direction == room_goes_horizontal) {
                    door_east = true;
                }
                else {
                    door_north = true;
                }
                
                add_std_room(game_state, 
                             screen_x * tiles_per_width + tiles_per_width / 2, 
                             screen_y * tiles_per_height + tiles_per_height / 2,
                             absolute_tile_z);
                
                for (u32 tile_y = 0; tile_y < tiles_per_height; tile_y++) {
                    for (u32 tile_x = 0; tile_x < tiles_per_width; tile_x++) {
                        u32 absolute_tile_x = screen_x * tiles_per_width + tile_x;
                        u32 absolute_tile_y = screen_y * tiles_per_height + tile_y;
                        
                        bool should_be_door = false;
                        
                        if (tile_x == 0 && (!door_west || tile_y != tiles_per_height / 2)) {
                            should_be_door = true;
                        }
                        
                        if (tile_x == tiles_per_width - 1 && (!door_east || tile_y != tiles_per_height / 2)) {
                            should_be_door = true;
                        }
                        
                        if (tile_y == 0 && (!door_south || tile_x != tiles_per_width / 2)) {
                            should_be_door = true;
                        }
                        
                        if (tile_y == tiles_per_height - 1 && (!door_north || tile_x != tiles_per_width / 2)) {
                            should_be_door = true;
                        }
                        
                        if (should_be_door) {
                            add_wall(game_state, absolute_tile_x, absolute_tile_y, absolute_tile_z);
                        }
                        else if (created_z_door) {
                            if (tile_y == 6 && tile_x == 6) {
                                
                                u32 door_z = door_down ? absolute_tile_z - 1 : absolute_tile_z;
                                add_stairs(game_state, absolute_tile_x, absolute_tile_y, door_z);
                            }
                        }
                        
                    }
                }
                
                door_west  = door_east;
                door_south = door_north;
                door_east  = false;
                door_north = false;
                
                if (created_z_door) {
                    door_down = !door_down;
                    door_up   = !door_up;
                }
                else {
                    door_down = false;
                    door_up   = false;
                }
                
                if (door_direction == room_has_stairs) {
                    if (absolute_tile_z == screen_base_z)
                        absolute_tile_z = screen_base_z + 1;
                    else
                        absolute_tile_z = screen_base_z;
                }
                else if (door_direction == room_goes_horizontal) {
                    screen_x++;
                }
                else {
                    screen_y++;
                }
            }
        }
        
        // camera initial position
        World_position new_campera_pos = {};
        u32 camera_tile_x = screen_base_x * tiles_per_width  + 17 / 2;
        u32 camera_tile_y = screen_base_y * tiles_per_height + 9 / 2;
        u32 camera_tile_z = screen_base_z;
        new_campera_pos = chunk_pos_from_tile_pos(game_state->world, camera_tile_x, camera_tile_y, camera_tile_z);
        
        game_state->camera_pos = new_campera_pos;
        
        add_monster(game_state, camera_tile_x + 1, camera_tile_y + 1, camera_tile_z);
        
        for (u32 familiar_index = 0; familiar_index < 1; familiar_index++) {
            i32 familiar_offset_x = random_between(&series, -5, 5);
            i32 familiar_offset_y = random_between(&series, -3, 3);
            
            if (familiar_offset_y != 0 || familiar_offset_x != 0)
                add_familiar(game_state, camera_tile_x + familiar_offset_x, camera_tile_y + familiar_offset_y, camera_tile_z);
        }
        
        memory->is_initialized = true;
    }
    
    // transient init
    macro_assert(sizeof(Transient_state) <= memory->transient_storage_size);
    Transient_state* tran_state = (Transient_state*)memory->transient_storage;
    if (!tran_state->is_initialized) {
        
        initialize_arena(&tran_state->tran_arena,
                         memory->transient_storage_size - sizeof(Transient_state),
                         (u8*)memory->transient_storage + sizeof(Transient_state));
        
        tran_state->ground_buffer_count = 128;
        tran_state->ground_buffer_list = mem_push_array(&tran_state->tran_arena, tran_state->ground_buffer_count, Ground_buffer);
        
        for (u32 ground_buffer_index = 0; ground_buffer_index < tran_state->ground_buffer_count; ground_buffer_index++) {
            Ground_buffer* ground_buffer = tran_state->ground_buffer_list + ground_buffer_index;
            tran_state->ground_buffer_template = make_empty_bitmap(&tran_state->tran_arena,
                                                                   ground_buffer_width,
                                                                   ground_buffer_height,
                                                                   false);
            ground_buffer->memory = tran_state->ground_buffer_template.memory;
            ground_buffer->position = null_position();
        }
        
        tran_state->is_initialized = true;
    }
    
    World* world = game_state->world;
    
    //f32 meters_to_pixels = game_state->meters_to_pixels;
    macro_assert(world);
    
    // check input and move player
    {
        for (i32 controller_index = 0; controller_index < macro_array_count(input->gamepad); controller_index++) {
            auto input_state = get_gamepad(input, controller_index);
            Controlled_hero* controlled = game_state->controlled_hero_list + controller_index;
            
            if (controlled->entity_index == 0) {
                if (input_state->start.ended_down) {
                    *controlled = {};
                    controlled->entity_index = add_player(game_state).low_index;
                }
            }
            else {
                controlled->dd_player = {};
                controlled->d_sword = {};
                controlled->d_z = {};
                controlled->boost = 0;;
                
                if (input_state->is_analog) {
                    controlled->dd_player = v2 {
                        input_state->stick_avg_x,
                        input_state->stick_avg_y
                    };
                } 
                else {
                    // digital
                    if (input_state->up.ended_down) {
                        controlled->dd_player.y = 1.0f;
                    }
                    if (input_state->down.ended_down) {
                        controlled->dd_player.y = -1.0f;
                    }
                    if (input_state->left.ended_down) {
                        controlled->dd_player.x = -1.0f;
                    }
                    if (input_state->right.ended_down) {
                        controlled->dd_player.x = 1.0f;
                    }
                }
                
                if (input_state->action.ended_down)
                    controlled->d_z = 4.0f;
                
                if (input_state->shift.ended_down)
                    controlled->boost = 10.0f;
                
                // sword input check
                {
                    if (input_state->action_up.ended_down) {
                        controlled->d_sword = { 0.0f, 1.0f };
                    }
                    if (input_state->action_down.ended_down) {
                        controlled->d_sword = { 0.0f, -1.0f };
                    }
                    if (input_state->action_left.ended_down) {
                        controlled->d_sword = { -1.0f, 0.0f };
                    }
                    if (input_state->action_right.ended_down) {
                        controlled->d_sword = { 1.0f, 0.0f };
                    }
                }
            }
        }
    }
    
    Loaded_bmp _draw_buffer = {};
    Loaded_bmp* draw_buffer = &_draw_buffer;
    draw_buffer->width  = bitmap_buffer->width;
    draw_buffer->height = bitmap_buffer->height;
    draw_buffer->pitch  = bitmap_buffer->pitch;
    draw_buffer->memory = bitmap_buffer->memory;
    
    f32 meters_to_pixels = game_state->meters_to_pixels;
    f32 pixels_to_meters = 1.0f / meters_to_pixels;
    
    v2 screen_center = { 
        (f32)draw_buffer->width  * 0.5f,
        (f32)draw_buffer->height * 0.5f
    };
    
    f32 screen_width_meters  = draw_buffer->width  * pixels_to_meters;
    f32 screen_height_meters = draw_buffer->height * pixels_to_meters;
    Rect3 camera_bounds_meters = rect_center_dim(v3 {0, 0, 0}, v3{screen_width_meters, screen_height_meters, 0.0f} );
    if (1)
    {
        World_position min_chunk_pos = map_into_chunk_space(world, game_state->camera_pos, camera_bounds_meters.min);
        World_position max_chunk_pos = map_into_chunk_space(world, game_state->camera_pos, camera_bounds_meters.max);
        
        for (i32 chunk_z = min_chunk_pos.chunk_z; chunk_z <= max_chunk_pos.chunk_z; chunk_z++) {
            for (i32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; chunk_y++) {
                for (i32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; chunk_x++) {
                    
                    //World_chunk* chunk = get_world_chunk(world, chunk_x, chunk_y, chunk_z);
                    
                    //if (!chunk)
                    //continue;
                    
                    World_position chunk_center_pos = centered_chunk_point(chunk_x, chunk_y, chunk_z);
                    v3 relative_pos = subtract_pos(world, &chunk_center_pos, &game_state->camera_pos);
                    v2 screen_pos =  {
                        screen_center.x + meters_to_pixels * relative_pos.x,
                        screen_center.y - meters_to_pixels * relative_pos.y,
                    };
                    v2 screen_dim = meters_to_pixels * world->chunk_dim_meters.xy;
                    
                    bool found = false;
                    Ground_buffer* empty_buffer = 0;
                    for (u32 i = 0; i < tran_state->ground_buffer_count; i++) {
                        Ground_buffer* ground_buffer = tran_state->ground_buffer_list + i;
                        
                        if (are_in_same_chunk(world, &ground_buffer->position, &chunk_center_pos)) {
                            found = true;
                            break;
                        }
                        else if (!is_position_valid(ground_buffer->position)) {
                            empty_buffer = ground_buffer;
                        }
                    }
                    
                    if (!found && empty_buffer) {
                        fill_ground_chunk(tran_state, game_state, empty_buffer, &chunk_center_pos);
                    }
                    
                    draw_rect_outline(draw_buffer,
                                      screen_pos - 0.5f * screen_dim,
                                      screen_pos + 0.5f * screen_dim, 
                                      yellow_v3);
                }
            }
        }
    }
    
    // sim
    Sim_region* sim_region = {};
    Temp_memory sim_memory = {};
    {
        v3 sim_bound_expansion = {15.0f, 15.0f, 15.0f};
        Rect3 sim_camera_bounds = add_radius_to(camera_bounds_meters, sim_bound_expansion);
        
        sim_memory = begin_temp_memory(&tran_state->tran_arena);
        sim_region = begin_sim(&tran_state->tran_arena, 
                               game_state, game_state->world, 
                               game_state->camera_pos, sim_camera_bounds, input->time_delta);
    }
    
    
    //clear_screen(draw_buffer, color_gray_byte);
    
    // draw background
    draw_bitmap(draw_buffer, &game_state->background, v2 {0, 0});
    
    // draw ground
    for (u32 ground_buffer_index = 0; ground_buffer_index < tran_state->ground_buffer_count; ground_buffer_index++) {
        Ground_buffer* ground_buffer = tran_state->ground_buffer_list + ground_buffer_index;
        
        if (!is_position_valid(ground_buffer->position))
            continue;
        
        Loaded_bmp bitmap = tran_state->ground_buffer_template;
        bitmap.memory = ground_buffer->memory;
        
        v3 delta = game_state->meters_to_pixels * subtract_pos(game_state->world, 
                                                               &ground_buffer->position, 
                                                               &game_state->camera_pos);
        
        v2 ground = {
            screen_center.x + delta.x - 0.5f * (f32)bitmap.width,
            screen_center.y - delta.y - 0.5f * (f32)bitmap.height
        };
        
        draw_bitmap(draw_buffer, &bitmap, ground);
    }
    
    // move, group and draw
    {
        Entity_visible_piece_group piece_group = {};
        piece_group.game_state = game_state;
        Sim_entity* entity = sim_region->entity_list;
        for (u32 entity_index = 0; entity_index < sim_region->entity_count; entity_index++, entity++) {
            
            if (!entity->updatable)
                continue;
            
            piece_group.count = 0;
            u32 i = entity_index;
            
            f32 time_delta = input->time_delta;
            
            v3 color_player = { .1f, .0f, 1.0f };
            
            Move_spec move_spec = default_move_spec();
            v3 ddp = {};
            
            // update entities
            switch (entity->type) {
                case Entity_type_hero: {
                    
                    u32 hero_count = macro_array_count(game_state->controlled_hero_list);
                    for (u32 control_index = 0; control_index < hero_count; control_index++) {
                        Controlled_hero* con_hero = game_state->controlled_hero_list + control_index;
                        
                        if (entity->storage_index == con_hero->entity_index) {
                            
                            if (con_hero->d_z != 0.0f) {
                                entity->velocity_d.z = con_hero->d_z;
                            }
                            
                            move_spec.max_acceleration_vector = true;
                            move_spec.speed = 50.0f;
                            move_spec.drag = 8.0f;
                            move_spec.boost = con_hero->boost;
                            ddp = v2_to_v3(con_hero->dd_player, 0);
                            
                            if (!is_zero(con_hero->d_sword)) {
                                Sim_entity* sword = entity->sword.pointer;
                                
                                if (sword && is_set(sword, Entity_flag_non_spatial)) {
                                    sword->distance_limit = 5.0f;
                                    make_entity_spatial(sword, entity->position, v2_to_v3(5.0f * con_hero->d_sword, 0));
                                    add_collision_rule(game_state, sword->storage_index, entity->storage_index, false);
                                }
                            }
                        }
                    }
                    
                    Hero_bitmaps* hero_bitmap = &game_state->hero_bitmaps[entity->facing_direction];
                    
                    /*
                    // jump
                    f32 jump_z;
                    {
                        
                        entity->z_velocity_d = ddz * time_delta + entity->z_velocity_d;
                        
                        
                        
                        jump_z = -entity->z;
                    }
                    */
                    push_bitmap(&piece_group, &hero_bitmap->hero_body, v2{0, 0}, 0, hero_bitmap->align);
                    
                    draw_hitpoints(&piece_group, entity);
                } break;
                
                case Entity_type_familiar: {
                    Sim_entity* closest_hero = 0;
                    // distance we want it start to follow
                    f32 closest_hero_delta_squared = square(10.0f);
                    bool follow_player = false;
                    
                    if (follow_player) {
                        Sim_entity* test_entity = sim_region->entity_list;
                        for (u32 test_entity_index = 0; test_entity_index < sim_region->entity_count; test_entity_index++, test_entity++) {
                            if (test_entity->type != Entity_type_hero)
                                continue;
                            
                            f32 test_delta_squared = length_squared(test_entity->position - entity->position);
                            
                            if (closest_hero_delta_squared > test_delta_squared) {
                                closest_hero = test_entity;
                                closest_hero_delta_squared = test_delta_squared;
                            }
                        }
                    }
                    
                    f32 distance_to_stop = 2.5f;
                    bool move_closer = closest_hero_delta_squared > square(distance_to_stop);
                    if (closest_hero && move_closer) {
                        f32 acceleration = 0.5f;
                        f32 one_over_length = acceleration / square_root(closest_hero_delta_squared);
                        ddp = one_over_length * (closest_hero->position - entity->position);
                    }
                    
                    move_spec.max_acceleration_vector = true;
                    move_spec.speed = 50.0f;
                    move_spec.drag = 8.0f;
                    move_spec.boost = 1.0f;
                    
                    v2 align = {34, 72};
                    entity->t_bob += time_delta;
                    if (entity->t_bob > 2.0f * PI) {
                        entity->t_bob -= 2.0f * PI;
                    }
                    push_bitmap(&piece_group, &game_state->familiar, v2{0, 0}, 0.5f * sin(entity->t_bob), align);
                } break;
                
                case Entity_type_monster: {
                    v2 align = {25, 42};
                    push_bitmap(&piece_group, &game_state->monster, v2{0, 0}, 0, align);
                    draw_hitpoints(&piece_group, entity);
                } break;
                
                case Entity_type_sword: {
                    
                    move_spec.max_acceleration_vector = false;
                    move_spec.speed = 0.0f;
                    move_spec.drag = 0.0f;
                    move_spec.boost = 0.0f;
                    
                    if (entity->distance_limit == 0.0f) {
                        clear_collision_rule(game_state, entity->storage_index);
                        make_entity_non_spatial(entity);
                    }
                    v2 align = {33, 29};
                    
                    push_bitmap(&piece_group, &game_state->sword, v2{0, 0}, 0, align);
                } break;
                
                case Entity_type_wall: {
                    //draw_rect(draw_buffer, player_start, player_end, color_player);
                    v2 align = {50, 115};
                    push_bitmap(&piece_group, &game_state->tree, v2{0, 0}, 0, align);
                } break;
                
                case Entity_type_stairwell: {
                    push_rect_outline(&piece_group, v2{0,0}, 0, entity->walkable_dim, v4 {1, .5f, 0, 1}, 0.0f);
                    push_rect_outline(&piece_group, v2{0,0}, entity->walkable_height, entity->walkable_dim, v4 {1, 1, 0, 1}, 0.0f);
                } break;
                
                case Entity_type_space: {
                    bool show_level_outlines = false;
                    
                    if (!show_level_outlines)
                        break;
                    
                    for (u32 volume_index = 0; volume_index < entity->collision->volume_count; volume_index++) {
                        Sim_entity_collision_volume* volume = entity->collision->volume_list + volume_index;
                        
                        push_rect_outline(&piece_group, volume->offset_pos.xy, 0, volume->dim.xy, v4 {0, .5f, 0, 1}, 0.0f);
                    }
                } break;
                
                default: {
                    macro_assert(!"INVALID");
                } break;
            }
            
            if (!is_set(entity, Entity_flag_non_spatial) &&
                is_set(entity, Entity_flag_moveable)) {
                move_entity(game_state, sim_region, entity, input->time_delta, &move_spec, ddp);
            }
            
            // actually draw
            for (u32 piece_index = 0; piece_index < piece_group.count; piece_index++) {
                
                Entity_visible_piece* piece = piece_group.piece_list + piece_index;
                
                v3 entity_base_point = get_entity_ground_point(entity);
                f32 z_fudge = 1.0f + 0.1f * (entity_base_point.z + piece->offset_z);
                
                v2 entity_ground_point = { 
                    screen_center.x + entity_base_point.x * z_fudge * meters_to_pixels,
                    screen_center.y - entity_base_point.y * z_fudge * meters_to_pixels
                };
                f32 entity_z = -meters_to_pixels * entity_base_point.z;
                
                v2 center = { 
                    entity_ground_point.x + piece->offset.x, 
                    entity_ground_point.y + piece->offset.y + piece->entity_zc * entity_z
                };
                
                if (piece->bitmap) {
                    draw_bitmap(draw_buffer, piece->bitmap, center, piece->color.a);
                }
                else {
                    v2 half_dim = 0.5f * meters_to_pixels * piece->dim;
                    v3 temp_color = color_v3_v4(piece->color);
                    draw_rect(draw_buffer, center - half_dim, center + half_dim, temp_color);
                }
            }
            
        }
    }
    
    World_position world_origin = {};
    v3 diff = subtract_pos(sim_region->world, &world_origin, &sim_region->origin);
    draw_rect(draw_buffer, diff.xy, v2 {10.0f, 10.0f}, v3{1.0f,1.0f,0.0f});
    end_sim(sim_region, game_state);
    
    end_temp_memory(sim_memory);
    check_arena(&game_state->world_arena);
    check_arena(&tran_state->tran_arena);
#if 0
    subpixel_test_update(draw_buffer, game_state, input, gold_v3);
#endif
    
#if 0
    drops_update(draw_buffer, game_state, input);
#endif
    
#if 0
    vectors_update(draw_buffer, game_state, input);
#endif
    
#if 0
    each_pixel(draw_buffer, game_state, input->time_delta);
#endif
}

#endif

extern "C" 
void 
game_get_sound_samples(thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer) {
    auto game_state = (Game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400, game_state);
}