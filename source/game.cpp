#include <stdio.h>

#include "types.h"
#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "intrinsics.h"

#include "game.h"
#include "render_group.h"
#include "render_group.cpp"
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
v2
top_down_align(Loaded_bmp* bitmap, v2 align) {
    align.y = (f32)(bitmap->height - 1) - align.y;
    
    align.x = safe_ratio_0(align.x, (f32)bitmap->width);
    align.y = safe_ratio_0(align.y, (f32)bitmap->height);
    
    return align;
}

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
debug_read_and_write_random_file() {
    auto bitmap_read = debug_read_entire_file(__FILE__);
    if (bitmap_read.content) {
        debug_write_entire_file("temp.cpp", bitmap_read.bytes_read, bitmap_read.content);
        debug_free_file(bitmap_read.content);
    }
}

internal
void
draw_pixel(Loaded_bmp* bitmap_buffer, v2 pos, v4 color) {
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
draw_circle(Loaded_bmp* bitmap_buffer, v2 start, f32 radius, v4 color) {
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
draw_line(Loaded_bmp* bitmap_buffer, v2 start, v2 end, v4 color, u32 points = 100) {
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
    
    draw_pixel(bitmap_buffer, { 0, 0 }, gold_v4);
    draw_pixel(bitmap_buffer, { 0, x }, red_v4);
    draw_pixel(bitmap_buffer, { x, 0 }, red_v4);
    draw_pixel(bitmap_buffer, { 0, -x }, red_v4);
    draw_pixel(bitmap_buffer, { -x, 0 }, red_v4);
    
    draw_line(bitmap_buffer, { 0, 0 }, vec, green_v4);
    draw_line(bitmap_buffer, { 0, 0 }, perpendicular(vec), green_v4);
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
    draw_pixel(bitmap_buffer, v2 { pixel_state->x, pixel_state->y }, red_v4);
}

internal
void
drops_update(Loaded_bmp* bitmap_buffer, Game_state* state, Game_input* input) {
    
    // draw mouse pointer
    f32 mouse_draw_offset = 10.0f; // offset from windows layer
    {
        v2 mouse_start = { 
            (f32)input->mouse_x - mouse_draw_offset,
            (f32)input->mouse_y - mouse_draw_offset 
        };
        
        v2 mouse_end = {
            (f32)input->mouse_x,
            (f32)input->mouse_y
        };
        
        draw_rect(bitmap_buffer, mouse_start, mouse_end, white_v4);
        
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
            
            draw_rect(bitmap_buffer, drop->pos, drop_end , white_v4);
        }
    }
}

#if 0
#include "pacman.cpp"
#endif

internal
Loaded_bmp
debug_load_bmp(char* file_name, i32 align_x, i32 top_down_align_y) {
    Loaded_bmp result = {};
    
    Debug_file_read_result file_result = debug_read_entire_file(file_name);
    
    macro_assert(file_result.bytes_read != 0);
    
    Bitmap_header* header = (Bitmap_header*)file_result.content;
    
    macro_assert(result.height >= 0);
    macro_assert(header->Compression == 3);
    
    result.memory = (u32*)((u8*)file_result.content + header->BitmapOffset);
    
    result.width  = header->Width;
    result.height = header->Height;
    result.align_pcent = top_down_align(&result, v2_i32(align_x, top_down_align_y));
    result.width_over_height = safe_ratio_0((f32)result.width, (f32)result.height);
    f32 pixel_to_meter = 1.0f / 42.0f;
    result.native_height = pixel_to_meter * result.height;
    
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
            
            v4 texel = {
                (f32)((color & mask_red)   >> shift_red_down),
                (f32)((color & mask_green) >> shift_green_down),
                (f32)((color & mask_blue)  >> shift_blue_down),
                (f32)((color & mask_alpha) >> shift_alpha_down),
            };
            
            texel = srgb255_to_linear1(texel);
            
            bool premultiply_alpha = true;
            if (premultiply_alpha) {
                texel.rgb *= texel.a;
            }
            
            texel = linear1_to_srgb255(texel);
            
            *source_dest++ = (((u32)(texel.a + 0.5f) << 24) | 
                              ((u32)(texel.r + 0.5f) << 16) | 
                              ((u32)(texel.g + 0.5f) << 8 ) | 
                              ((u32)(texel.b + 0.5f) << 0 ));
        }
    }
    
    i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
    // top-to-bottom
#if 0
    result.pitch = -result.width * bytes_per_pixel;
    result.memory = (u8*)result.memory - result.pitch * (result.height - 1);
#else
    // bottom-to-top
    result.pitch = result.width * bytes_per_pixel;
#endif
    
    return result;
}

internal
Loaded_bmp
debug_load_bmp(char* file_name) {
    Loaded_bmp result = debug_load_bmp(file_name, 0, 0);
    result.align_pcent = v2{0.5f, 0.5f};
    
    return result;
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
draw_hitpoints(Render_group* piece_group, Sim_entity* entity) {
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
            
            push_rect(piece_group, v2_to_v3(hit_p, 0), health_dim, color);
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
    
    Temp_memory ground_memory = begin_temp_memory(&tran_state->tran_arena);
    
    Loaded_bmp* bitmap_buffer = &ground_buffer->bitmap;
    
    bitmap_buffer->align_pcent = { 0.5f, 0.5f };
    bitmap_buffer->width_over_height = 1.0f;
    
    Render_group* render_group = allocate_render_group(&tran_state->tran_arena, macro_megabytes(4), bitmap_buffer->width, bitmap_buffer->height);
    
    push_clear(render_group, yellow_v4);
    
    ground_buffer->position = *chunk_pos;
    
#if 1
    f32 width  = game_state->world->chunk_dim_meters.x;
    f32 height = game_state->world->chunk_dim_meters.y;
    
    v2 half_dim = 0.5f * v2{width, height};
    half_dim = 2.0f * half_dim;
    
    u32 random_number_index = 0;
    for (i32 chunk_offset_y = -1; chunk_offset_y <= 1; chunk_offset_y++) {
        for (i32 chunk_offset_x = -1; chunk_offset_x <= 1; chunk_offset_x++) {
            
            i32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
            i32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
            i32 chunk_z = chunk_pos->chunk_z;
            
            Random_series series = random_seed(139 * chunk_x + 593 * chunk_y + 329 * chunk_z);
            
            v2 center = v2 {chunk_offset_x * width, chunk_offset_y * height};
            
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
                
                v2 pos = 
                    center + 
                    hadamard(half_dim, v2 { random_bilateral(&series), random_bilateral(&series) });
                
                push_bitmap(render_group, stamp, 4.0f, v2_to_v3(pos, 0.0f));
            }
        }
    }
    
    for (i32 chunk_offset_y = -1; chunk_offset_y <= 1; chunk_offset_y++) {
        for (i32 chunk_offset_x = -1; chunk_offset_x <= 1; chunk_offset_x++) {
            
            i32 chunk_x = chunk_pos->chunk_x + chunk_offset_x;
            i32 chunk_y = chunk_pos->chunk_y + chunk_offset_y;
            i32 chunk_z = chunk_pos->chunk_z;
            
            Random_series series = random_seed(139 * chunk_x + 593 * chunk_y + 329 * chunk_z);
            
            v2 center = v2 {chunk_offset_x * width, chunk_offset_y * height};
            
            for (u32 grass_index = 0; grass_index < 50; grass_index++) {
                macro_assert(random_number_index < macro_array_count(random_number_table));
                
                Loaded_bmp* stamp;
                
                u32 count = macro_array_count(game_state->tuft);
                u32 random_index = random_choise(&series, count);
                stamp = game_state->tuft + random_index;
                
                v2 pos = 
                    center + 
                    hadamard(half_dim, v2 { random_bilateral(&series), random_bilateral(&series) });
                
                push_bitmap(render_group, stamp, 0.4f, v2_to_v3(pos, 0.0f));
            }
        }
    }
#endif
    render_group_to_output(render_group, bitmap_buffer);
    end_temp_memory(ground_memory);
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
    result.memory = mem_push_size(arena, total_bitmap_size);
    
    if (clear_to_zero)
        clear_bitmap(&result);
    
    return result;
}


internal
void
make_pyramid_normal_map(Loaded_bmp* bitmap, f32 roughness) {
    
    f32 inv_width  = 1.0f / (f32)(bitmap->width  - 1);
    f32 inv_height = 1.0f / (f32)(bitmap->height - 1);
    
    u8* row = (u8*)bitmap->memory;
    
    for (i32 y = 0; y < bitmap->height; y++) {
        u32* pixel = (u32*)row;
        for (i32 x = 0; x < bitmap->width; x++) {
            
            v2 bitmap_uv = {
                inv_width  * (f32)x,
                inv_height * (f32)y,
            };
            
            i32 inv_x = bitmap->width - 1 - x;
            f32 normalized = 1.0f / sqrtf(1 + 1);
            v3 normal = {0,0,normalized};
            if (x < y) {
                if (inv_x < y) {
                    normal.x = -normalized;
                }
                else  {
                    normal.y = normalized;
                }
            }
            else {
                if (inv_x < y) {
                    normal.y = -normalized;
                }
                else {
                    normal.x = normalized;
                }
            }
            
            v4 color = {
                255.0f * (0.5f * (normal.x + 1.0f)),
                255.0f * (0.5f * (normal.y + 1.0f)),
                255.0f * (0.5f * (normal.z + 1.0f)),
                255.0f * roughness
            };
            
            *pixel++ = ((u32)(color.a + 0.5f) << 24 | 
                        (u32)(color.r + 0.5f) << 16 | 
                        (u32)(color.g + 0.5f) << 8  |
                        (u32)(color.b + 0.5f) << 0);
            
        }
        row += bitmap->pitch;
    }
}

internal
void
make_sphere_diffuse_map(Loaded_bmp* bitmap, f32 cx = 1.0f, f32 cy = 1.0f) {
    
    f32 inv_width  = 1.0f / (f32)(bitmap->width  - 1);
    f32 inv_height = 1.0f / (f32)(bitmap->height - 1);
    
    u8* row = (u8*)bitmap->memory;
    
    for (i32 y = 0; y < bitmap->height; y++) {
        u32* pixel = (u32*)row;
        for (i32 x = 0; x < bitmap->width; x++) {
            
            v2 bitmap_uv = {
                inv_width  * (f32)x,
                inv_height * (f32)y,
            };
            
            f32 nx = cx * (2.0f * bitmap_uv.x - 1.0f);
            f32 ny = cy * (2.0f * bitmap_uv.y - 1.0f);
            
            f32 alpha = 0.0f;
            f32 root_term = 1.0f - nx*nx - ny*ny;
            
            if (root_term >= 0.0f) {
                alpha = 1.0f;
            }
            
            v3 base_color = black_v3;
            alpha *= 255.0f;
            v4 color = {
                alpha * base_color.x,
                alpha * base_color.y,
                alpha * base_color.z,
                alpha
            };
            
            *pixel++ = ((u32)(color.a + 0.5f) << 24 | 
                        (u32)(color.r + 0.5f) << 16 | 
                        (u32)(color.g + 0.5f) << 8  |
                        (u32)(color.b + 0.5f) << 0);
            
        }
        row += bitmap->pitch;
    }
}


internal
void
make_sphere_normal_map(Loaded_bmp* bitmap, f32 roughness, f32 cx = 1.0f, f32 cy = 1.0f) {
    
    f32 inv_width  = 1.0f / (f32)(bitmap->width  - 1);
    f32 inv_height = 1.0f / (f32)(bitmap->height - 1);
    
    u8* row = (u8*)bitmap->memory;
    
    for (i32 y = 0; y < bitmap->height; y++) {
        u32* pixel = (u32*)row;
        for (i32 x = 0; x < bitmap->width; x++) {
            
            v2 bitmap_uv = {
                inv_width  * (f32)x,
                inv_height * (f32)y,
            };
            
            f32 nx = cx * (2.0f * bitmap_uv.x - 1.0f);
            f32 ny = cy * (2.0f * bitmap_uv.y - 1.0f);
            
            f32 root_term = 1.0f - nx*nx - ny*ny;
            f32 normalized = 1.0f / sqrtf(1 + 1);
            v3 normal = {0,normalized,normalized};
            f32 nz = 0.0f;
            
            if (root_term >= 0.0f) {
                nz = square_root(root_term);
                normal = { nx, ny, nz };
            }
            
            v4 color = {
                255.0f * (0.5f * (normal.x + 1.0f)),
                255.0f * (0.5f * (normal.y + 1.0f)),
                255.0f * (0.5f * (normal.z + 1.0f)),
                255.0f * roughness
            };
            
            *pixel++ = ((u32)(color.a + 0.5f) << 24 | 
                        (u32)(color.r + 0.5f) << 16 | 
                        (u32)(color.g + 0.5f) << 8  |
                        (u32)(color.b + 0.5f) << 0);
            
        }
        row += bitmap->pitch;
    }
}

internal
void
make_cylinder_normal_map_x(Loaded_bmp* bitmap, f32 roughness) {
    
    f32 inv_width  = 1.0f / (f32)(bitmap->width  - 1);
    f32 inv_height = 1.0f / (f32)(bitmap->height - 1);
    
    u8* row = (u8*)bitmap->memory;
    
    for (i32 y = 0; y < bitmap->height; y++) {
        u32* pixel = (u32*)row;
        for (i32 x = 0; x < bitmap->width; x++) {
            
            v2 bitmap_uv = {
                inv_width  * (f32)x,
                inv_height * (f32)y,
            };
            
            f32 nx = 2.0f * bitmap_uv.x - 1.0f;
            f32 ny = 2.0f * bitmap_uv.y - 1.0f;
            
            f32 root_term = 1.0f - nx*nx;
            f32 normalized = 1.0f / sqrtf(1 + 1);
            v3 normal = {0,normalized,normalized};
            f32 nz = 0.0f;
            
            macro_assert(root_term >= 0.0f);
            
            nz = square_root(root_term);
            normal = { nx, ny, nz };
            
            v4 color = {
                255.0f * (0.5f * (normal.x + 1.0f)),
                255.0f * (0.5f * (normal.y + 1.0f)),
                255.0f * (0.5f * (normal.z + 1.0f)),
                255.0f * roughness
            };
            
            *pixel++ = ((u32)(color.a + 0.5f) << 24 | 
                        (u32)(color.r + 0.5f) << 16 | 
                        (u32)(color.g + 0.5f) << 8  |
                        (u32)(color.b + 0.5f) << 0);
            
        }
        row += bitmap->pitch;
    }
}

#if INTERNAL
Game_memory* debug_global_memory;
#endif

extern "C"
void 
game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
#if INTERNAL
    debug_global_memory = memory;
    auto start_cycle_count = __rdtsc();
#endif
    
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
        
        const f32 pixels_to_meters = 1.0f / 42.0f;
        v3 world_chunk_dim_meters = {
            (f32)ground_buffer_width * pixels_to_meters,
            (f32)ground_buffer_height * pixels_to_meters,
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
            game_state->tree       = debug_load_bmp("../data/tree.bmp", 50, 115);
            game_state->monster    = debug_load_bmp("../data/george-front-0.bmp", 25, 42);
            game_state->familiar   = debug_load_bmp("../data/ship.bmp", 34, 72);
            game_state->sword      = debug_load_bmp("../data/sword.bmp", 33, 29);
            game_state->stairwell  = debug_load_bmp("../data/george-back-0.bmp");
            
#if 0
            // load blender guy
            {
                game_state->hero_bitmaps[0].hero_body = debug_load_bmp("../data/right.bmp", 38, 119);
                
                game_state->hero_bitmaps[1].hero_body = debug_load_bmp("../data/back.bmp", 40, 112);
                
                game_state->hero_bitmaps[2].hero_body = debug_load_bmp("../data/left.bmp", 40, 119);
                
                game_state->hero_bitmaps[3].hero_body = debug_load_bmp("../data/front.bmp", 39, 115);
            }
#else
            Hero_bitmaps* bitmap = &game_state->hero_bitmaps[0];
            bitmap->head  = debug_load_bmp("../data/test_hero_right_head.bmp", 72, 182);
            bitmap->cape  = debug_load_bmp("../data/test_hero_right_cape.bmp", 72, 182);
            bitmap->torso = debug_load_bmp("../data/test_hero_right_torso.bmp", 72, 182);
            
            bitmap++;
            bitmap->head  = debug_load_bmp("../data/test_hero_back_head.bmp", 72, 182);
            bitmap->cape  = debug_load_bmp("../data/test_hero_back_cape.bmp", 72, 182);
            bitmap->torso = debug_load_bmp("../data/test_hero_back_torso.bmp", 72, 182);
            
            bitmap = &game_state->hero_bitmaps[2];
            bitmap->head  = debug_load_bmp("../data/test_hero_left_head.bmp", 72, 182);
            bitmap->cape  = debug_load_bmp("../data/test_hero_left_cape.bmp", 72, 182);
            bitmap->torso = debug_load_bmp("../data/test_hero_left_torso.bmp", 72, 182);
            
            bitmap++;
            bitmap->head  = debug_load_bmp("../data/test_hero_front_head.bmp", 72, 182);
            bitmap->cape  = debug_load_bmp("../data/test_hero_front_cape.bmp", 72, 182);
            bitmap->torso = debug_load_bmp("../data/test_hero_front_torso.bmp", 72, 182);
            
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
            const u32 room_goes_horizontal = 1;
            const u32 stairs_up = 2;
            const u32 stairs_down = 3;
            
            bool door_north = false;
            bool door_east  = false;
            bool door_south = false;
            bool door_west  = false;
            bool door_up    = false;
            bool door_down  = false;
            
            const u32 max_screens = 2000;
            
            for (u32 screen_index = 0; screen_index < max_screens; screen_index++) {
#if 1
                u32 door_direction = random_choise(&series, (door_up || door_down) ? 2 : 3);
#else
                u32 door_direction = random_choise(&series, 2);
#endif
                // door_direction = 3;
                
                bool created_z_door = false;
                
                if (door_direction == stairs_up) {
                    created_z_door = true;
                    door_up = true;
                }
                else if (door_direction == stairs_down) {
                    created_z_door = true;
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
                            if (absolute_tile_z % 2 && tile_y == 6 && tile_x == 6 ||
                                !(absolute_tile_z % 2) && tile_y == 6 && tile_x == 3)
                            {
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
                
                if (door_direction == stairs_down) {
                    absolute_tile_z -= 1;
                }
                else if (door_direction == stairs_up) {
                    absolute_tile_z += 1;
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
        
        tran_state->ground_buffer_count = 256;
        tran_state->ground_buffer_list = mem_push_array(&tran_state->tran_arena, tran_state->ground_buffer_count, Ground_buffer);
        
        for (u32 ground_buffer_index = 0; ground_buffer_index < tran_state->ground_buffer_count; ground_buffer_index++) {
            Ground_buffer* ground_buffer = tran_state->ground_buffer_list + ground_buffer_index;
            ground_buffer->bitmap = make_empty_bitmap(&tran_state->tran_arena,
                                                      ground_buffer_width,
                                                      ground_buffer_height,
                                                      false);
            ground_buffer->position = null_position();
        }
        
        game_state->test_diffuse = make_empty_bitmap(&tran_state->tran_arena, 256, 256, false);
        draw_rect(&game_state->test_diffuse, v2{0,0}, v2_i32(game_state->test_diffuse.width, game_state->test_diffuse.height), v4{0.5f, 0.5f, 0.5f, 1.0f});
        
        game_state->test_normal = make_empty_bitmap(&tran_state->tran_arena, 
                                                    game_state->test_diffuse.width, game_state->test_diffuse.height, false);
        
        make_sphere_normal_map(&game_state->test_normal, 0.0f);
        make_sphere_diffuse_map(&game_state->test_diffuse);
        //make_sphere_normal_map(&game_state->test_normal, 0.0f, 0.1f, 1.0f); // cylinder
        //make_pyramid_normal_map(&game_state->test_normal, 0.0f);
        
        tran_state->env_map_width = 512;
        tran_state->env_map_height = 256;
        
        for (u32 map_index = 0; map_index < macro_array_count(tran_state->env_map_list); map_index += 1) {
            Env_map* map = tran_state->env_map_list + map_index;
            u32 width = tran_state->env_map_width;
            u32 height = tran_state->env_map_height;
            for (u32 lod_index = 0; lod_index < macro_array_count(map->lod); lod_index++) {
                map->lod[lod_index] = make_empty_bitmap(&tran_state->tran_arena, width, height, false);
                
                width >>= 1;
                height >>= 1;
            }
        }
        
        tran_state->is_initialized = true;
    }
    
#if 0
    if (input->executable_reloaded) {
        for (u32 ground_buffer_index = 0; ground_buffer_index < tran_state->ground_buffer_count; ground_buffer_index++) {
            Ground_buffer* ground_buffer = tran_state->ground_buffer_list + ground_buffer_index;
            ground_buffer->position = null_position();
        }
    }
#endif
    
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
    
    Temp_memory render_memory = begin_temp_memory(&tran_state->tran_arena);
    Render_group* render_group = allocate_render_group(&tran_state->tran_arena, macro_megabytes(4), draw_buffer->width, draw_buffer->height);
    
    // clear screen
    push_clear(render_group, grey_v4);
    
    v2 screen_center = { 
        (f32)draw_buffer->width  * 0.5f,
        (f32)draw_buffer->height * 0.5f
    };
    
    Rect2 screen_bounds = get_cam_rect_at_target(render_group);
    
    // camera bounds - sim bounds
    Rect3 camera_bounds_meters;
    {
        v3 center = v2_to_v3(screen_bounds.min, 0.0f);
        v3 dim = v2_to_v3(screen_bounds.max, 0.0f);;
        
        camera_bounds_meters = rect_min_max(center, dim);
        
        camera_bounds_meters.min.z = -3.0f * game_state->typical_floor_height;
        camera_bounds_meters.max.z =  1.0f * game_state->typical_floor_height;
    }
    
    // render ground chunks
    for (u32 ground_buffer_index = 0; ground_buffer_index < tran_state->ground_buffer_count; ground_buffer_index++) {
        Ground_buffer* ground_buffer = tran_state->ground_buffer_list + ground_buffer_index;
        
        if (!is_position_valid(ground_buffer->position))
            continue;
        
        Loaded_bmp* bitmap = &ground_buffer->bitmap;
        
        v3 delta = subtract_pos(game_state->world, 
                                &ground_buffer->position, 
                                &game_state->camera_pos);
        
        if (delta.z >= 1.0f || delta.z <= -1.0f)
            continue;
        
        Render_basis* basis = mem_push_struct(&tran_state->tran_arena, Render_basis);
        render_group->default_basis = basis;
        basis->position = delta;
        
        f32 ground_side_meters = world->chunk_dim_meters.x;
        push_bitmap(render_group, bitmap, ground_side_meters, v3{0,0,0});
        
        bool show_chunk_outlines = true;
        
        if (show_chunk_outlines) {
            push_rect_outline(render_group,
                              v3{0,0,0},
                              v2{ground_side_meters,ground_side_meters},
                              yellow_v4);
        }
    }
    
    // update ground chunks for sim region
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
                    
                    f32 furthest_buffer_len_sq = 0.0f;
                    Ground_buffer* furthest_buffer = 0;
                    for (u32 i = 0; i < tran_state->ground_buffer_count; i++) {
                        Ground_buffer* ground_buffer = tran_state->ground_buffer_list + i;
                        
                        if (are_in_same_chunk(world, &ground_buffer->position, &chunk_center_pos)) {
                            furthest_buffer = 0;
                            break;
                        }
                        else if (is_position_valid(ground_buffer->position)) {
                            
                            v3 rel_pos = subtract_pos(world, &ground_buffer->position, &game_state->camera_pos);
                            f32 buffer_length_sq = length_squared(rel_pos.xy);
                            
                            if (furthest_buffer_len_sq < buffer_length_sq) {
                                furthest_buffer_len_sq = buffer_length_sq;
                                furthest_buffer = ground_buffer;
                            }
                        }
                        else {
                            furthest_buffer_len_sq = MAX_F32;
                            furthest_buffer = ground_buffer;
                        }
                    }
                    
                    if (furthest_buffer) {
                        fill_ground_chunk(tran_state, game_state, furthest_buffer, &chunk_center_pos);
                    }
                }
            }
        }
    }
    
    // sim
    Sim_region* sim_region = {};
    Temp_memory sim_memory = {};
    Rect3 sim_camera_bounds = {};
    {
        v3 sim_bound_expansion = {15.0f, 15.0f, 0};
        sim_camera_bounds = add_radius_to(camera_bounds_meters, sim_bound_expansion);
        
        sim_memory = begin_temp_memory(&tran_state->tran_arena);
        sim_region = begin_sim(&tran_state->tran_arena, 
                               game_state, game_state->world, 
                               game_state->camera_pos, sim_camera_bounds, input->time_delta);
    }
    
    Render_basis* main_basis = mem_push_struct(&tran_state->tran_arena, Render_basis);
    main_basis->position = v3{0,0,0};
    render_group->default_basis = main_basis;
    
    // draw debug bounds
    {
        push_rect_outline(render_group, v3{0,0,0}, get_dim(screen_bounds), yellow_v4);
        //push_rect_outline(render_group, v3{0,0,0}, get_dim(camera_bounds_meters).xy, white_v4);
        push_rect_outline(render_group, v3{0,0,0}, get_dim(sim_camera_bounds).xy, cyan_v4);
        push_rect_outline(render_group, v3{0,0,0}, get_dim(sim_region->bounds).xy, purple_v4);
    }
    
    // move, group and push to drawing pipe
    {
        v3 camera_pos = subtract_pos(world, &game_state->camera_pos, &game_state->camera_pos);
        
        for (u32 entity_index = 0; entity_index < sim_region->entity_count; entity_index++) {
            
            Sim_entity* entity = sim_region->entity_list + entity_index;
            
            if (!entity->updatable)
                continue;
            
            u32 i = entity_index;
            
            f32 time_delta = input->time_delta;
            
            v3 color_player = { .1f, .0f, 1.0f };
            
            Move_spec move_spec = default_move_spec();
            v3 ddp = {};
            
            Render_basis* basis = mem_push_struct(&tran_state->tran_arena, Render_basis);
            render_group->default_basis = basis;
            
            // fadding out things based on camera z
            {
                v3 cam_relative_ground_pos = get_entity_ground_point(entity) - camera_pos;
                f32 fade_top_end_z = 1.0f * game_state->typical_floor_height;
                f32 fade_top_start_z = 0.5f * game_state->typical_floor_height;
                
                f32 fade_bot_start_z = -2.0f * game_state->typical_floor_height;
                f32 fade_bot_end_z = -2.25f * game_state->typical_floor_height;
                render_group->global_alpha = 1.0f;
                
                if (cam_relative_ground_pos.z > fade_top_start_z) {
                    render_group->global_alpha = clamp_map_to_range(fade_top_end_z, cam_relative_ground_pos.z, fade_top_start_z);
                }
                else if(cam_relative_ground_pos.z < fade_bot_start_z) {
                    render_group->global_alpha = clamp_map_to_range(fade_bot_end_z, cam_relative_ground_pos.z, fade_bot_start_z);
                }
            }
            
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
                    f32 hero_size = 2.5f;
                    push_bitmap(render_group, &hero_bitmap->torso, hero_size * 1.2f, v3{0, 0, 0});
                    push_bitmap(render_group, &hero_bitmap->cape, hero_size * 1.2f, v3{0, 0, 0});
                    push_bitmap(render_group, &hero_bitmap->head, hero_size * 1.2f, v3{0, 0, 0});
                    
                    draw_hitpoints(render_group, entity);
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
                    
                    entity->t_bob += time_delta;
                    if (entity->t_bob > 2.0f * PI) {
                        entity->t_bob -= 2.0f * PI;
                    }
                    v3 offset = {0, 0, 0.5f * sin(entity->t_bob)};
                    push_bitmap(render_group, &game_state->familiar, 1.0f, offset);
                } break;
                
                case Entity_type_monster: {
                    push_bitmap(render_group, &game_state->monster, 1.0f, v3{0, 0, 0});
                    draw_hitpoints(render_group, entity);
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
                    
                    push_bitmap(render_group, &game_state->sword, 1.0f, v3{0, 0, 0});
                } break;
                
                case Entity_type_wall: {
                    //draw_rect(draw_buffer, player_start, player_end, color_player);
                    
                    push_bitmap(render_group, &game_state->tree, 2.5f, v3{0,0,0});
                } break;
                
                case Entity_type_stairwell: {
                    push_rect_outline(render_group, v3{0,0,0}, entity->walkable_dim, v4 {1, .5f, 0, 1});
                    push_rect_outline(render_group, v3{0,0,entity->walkable_height}, entity->walkable_dim, v4 {1, 1, 0, 1});
                } break;
                
                case Entity_type_space: {
                    bool show_level_outlines = true;
                    
                    if (!show_level_outlines)
                        break;
                    
                    for (u32 volume_index = 0; volume_index < entity->collision->volume_count; volume_index++) {
                        Sim_entity_collision_volume* volume = entity->collision->volume_list + volume_index;
                        
                        push_rect_outline(render_group, volume->offset_pos - v3{0,0,0.5f*volume->dim.z}, volume->dim.xy, red_v4);
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
            
            basis->position = get_entity_ground_point(entity);
        }
    }
    
    // lightning mapping test
#if 0
    // prepare maps
    {
        v3 map_color[] = {
            {1,0,0},
            {0,1,0},
            {0,0,1},
        };
        
        for (u32 map_index = 0; map_index < macro_array_count(tran_state->env_map_list); map_index++) {
            Env_map* map = tran_state->env_map_list + map_index;
            Loaded_bmp* lod = map->lod + 0;
            bool row_checker_on = false;
            i32 checker_width = 16;
            i32 checker_height = 16;
            
            for (i32 y = 0; y < lod->height; y += checker_height) {
                bool checker_on = row_checker_on;
                for (i32 x = 0; x < lod->width; x += checker_width) {
                    v4 color = checker_on ? to_v4(map_color[map_index], 1.0f) : v4{0,0,0,1};
                    v2 min_pos = v2_i32(x, y);
                    v2 max_pos = min_pos + v2_i32(checker_width, checker_height);
                    draw_rect(lod, min_pos, max_pos, color);
                    checker_on = !checker_on;
                }
                row_checker_on = !row_checker_on;
            }
        }
    }
    
    f32 zoom_in_on_maps = 1.5f;
    tran_state->env_map_list[0].pos_z = -zoom_in_on_maps;
    tran_state->env_map_list[1].pos_z = 0.0f;
    tran_state->env_map_list[2].pos_z = zoom_in_on_maps;
    
    game_state->time += input->time_delta;
    f32 angle = 0.5f * game_state->time;
    bool move_around = true;
    
    v2 dis = { 
        150.0f * cos(angle),
        150.0f * sin(angle),
    };
    
    if (!move_around) 
        dis = {0,0};
    
    v2 origin = screen_center;
#if 1
    v2 x_axis = 100.0f * v2{cos(angle), sin(angle)};
    v2 y_axis = perpendicular(x_axis);
    
    v4 color = {
        0.5f + 0.5f*sin(angle),
        0.5f + 0.5f*sin(2.9f*angle),
        0.5f + 0.5f*cos(10.0f*angle),
        0.5f + 0.5f*cos(20.0f*angle)
    };
    
#else
    v2 x_axis = {100.0f, 0.0f};
    v2 y_axis = {0.0f, 100.0f};
    v4 color = {1.0f,1.0f,1.0f,1.0f};
    
#endif
    color = {1.0f,1.0f,1.0f,1.0f};
    //v2 x_axis = (50.0f + 50.0f * cos(angle)) * v2{cos(angle),sin(angle)};
    //v2 y_axis = (50.0f + 50.0f * cos(angle)) * v2{cos(angle + 1.0f), sin(angle + 1.0f)};
    
    v2 pos = dis + origin - 0.5f*x_axis - 0.5f*y_axis;
    push_coord_system(render_group, pos, x_axis, y_axis, color, 
                      &game_state->test_diffuse, 
                      &game_state->test_normal, 
                      tran_state->env_map_list + 2,
                      tran_state->env_map_list + 1,
                      tran_state->env_map_list + 0);
    
    v2 map_pos = { 0.0f, 0.0f };
    for (u32 map_index = 0; map_index < macro_array_count(tran_state->env_map_list); map_index++) {
        
        Env_map* map = tran_state->env_map_list + map_index;
        Loaded_bmp* lod = &map->lod[0];
        
        x_axis = 0.5f * v2{(f32)lod->width, 0.0f };
        y_axis = 0.5f * v2{ 0.0f, (f32)lod->height };
        
        v4 full_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        push_coord_system(render_group, map_pos, x_axis, y_axis, full_color, lod, 0, 0, 0, 0);
        map_pos += y_axis + v2{0.0f, 5.0f};
    }
#endif
    
#if 0
    push_saturation(render_group, 0.5f + 0.5f * sin(10.0f * game_state->time));
#endif 
    
    // output buffers to bitmap
    render_group_to_output(render_group, draw_buffer);
    
    end_sim(sim_region, game_state);
    
    end_temp_memory(sim_memory);
    end_temp_memory(render_memory);
    
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
    
#if INTERNAL
    debug_end_timer(Debug_cycle_counter_type_game_update_render, start_cycle_count);
#endif
}

#endif

extern "C" 
void 
game_get_sound_samples(thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer) {
    auto game_state = (Game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400, game_state);
}