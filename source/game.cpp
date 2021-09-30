#include <stdio.h>

#include "types.h"
#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "intrinsics.h"
#include "game.h"
#include "tile.h"
#include "tile.cpp"
#include "random.h"
#include "color.h"
#include "vectors.h"

void clear_screen(Game_bitmap_buffer* bitmap_buffer, u32 color) {
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

void render_255_gradient(Game_bitmap_buffer* bitmap_buffer, i32 blue_offset, i32 green_offset) {
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
void game_output_sound(Game_sound_buffer* sound_buffer, i32 tone_hz, Game_state* state) {
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
void draw_rect(Game_bitmap_buffer* buffer, v2 start, v2 end, v3 color) {
    u32 color_hex = 0;
    
    color_hex = (round_f32_u32(color.r * 255.0f) << 16 | 
                 round_f32_u32(color.g * 255.0f) << 8  |
                 round_f32_u32(color.b * 255.0f) << 0);
    
    u32 x0 = round_f32_u32(start.x); u32 x1 = round_f32_u32(end.x);
    u32 y0 = round_f32_u32(start.y); u32 y1 = round_f32_u32(end.y);
    
    x0 = clamp_i32(x0, 0, buffer->width);  x1 = clamp_i32(x1, 0, buffer->width);
    y0 = clamp_i32(y0, 0, buffer->height); y1 = clamp_i32(y1, 0, buffer->height);
    
    u8* row = (u8*)buffer->memory + x0 * buffer->bytes_per_pixel + y0 * buffer->pitch;
    
    for (u32 y = y0; y < y1; y++) {
        u32* pixel = (u32*)row;
        for (u32 x = x0; x < x1; x++) {
            *pixel++ = color_hex;
        }
        row += buffer->pitch;
    }
}

void debug_read_and_write_random_file() {
    auto bitmap_read = debug_read_entire_file(__FILE__);
    if (bitmap_read.content) {
        debug_write_entire_file("temp.cpp", bitmap_read.bytes_read, bitmap_read.content);
        debug_free_file(bitmap_read.content);
    }
}


internal
void draw_pixel(Game_bitmap_buffer* bitmap_buffer, v2 pos, v3 color) {
    v2 end = { pos.x + 1.0f, pos.y + 1.0f };
    
    draw_rect(bitmap_buffer, pos, end, color);
}

internal
void draw_circle(Game_bitmap_buffer* bitmap_buffer, v2 start, f32 radius, v3 color) {
    v2 screen_center = { 
        (f32)bitmap_buffer->width / 2,
        (f32)bitmap_buffer->height / 2 
    };
    
    start.y = -start.y;
    
    // y = sin (angle) * r
    // x = cos (angle) * r
    for (f32 angle = 0; angle < 360; angle++) {
        v2 v = { 
            cos(angle) * radius, 
            sin(angle) * radius 
        };
        draw_pixel(bitmap_buffer, v + screen_center + start, color);
    }
    
}

internal
void draw_line(Game_bitmap_buffer* bitmap_buffer, v2 start, v2 end, v3 color) {
    v2 screen_center = { 
        (f32)bitmap_buffer->width / 2,
        (f32)bitmap_buffer->height / 2 
    };
    
    start.y = -start.y;
    end.y = -end.y;
    
    start += screen_center;
    end += screen_center;
    
    f32 m = (end.y - start.y) / (end.x - start.x);
    // m - slope
    // b - intercept
    
    // y - y1 = m(x - x1)
    // y = m(x - x1) + y1
    v2 vector = end - start;
    
    // sqrt(x^2 + y^2)
    f32 vector_len = (f32)sqrt(square(vector.x) + square(vector.y));
    
    if (end.x > start.x) {
        for (f32 x = start.x; x <= end.x; x += 0.1f) {
            f32 y = m * (x - start.x) + start.y;
            v2 pixel = { x, y };
            draw_pixel(bitmap_buffer, pixel, color);
        }
    }
    else {
        for (f32 x = start.x; x >= end.x; x -= 0.1f) {
            f32 y = m * (x - start.x) + start.y;
            v2 pixel = { x, y };
            draw_pixel(bitmap_buffer, pixel, color);
        }
    }
}

internal
void vectors_update(Game_bitmap_buffer* bitmap_buffer, Game_state* state, Game_input* input) {
    clear_screen(bitmap_buffer, color_gray_byte);
    
    f32 x = 100;
    
    v2 red_vec   = { x, x };
    v2 green_vec = { -x, x };
    v2 blue_vec  = { -x, -x };
    v2 white_vec = { x, -x };
    
    v2 vec = white_vec;
    
    draw_line(bitmap_buffer, { 0, 0 }, vec, red_v3);
    draw_line(bitmap_buffer, { 0, 0 }, perpendicular_v2(vec), green_v3);
    
    draw_circle(bitmap_buffer, { 50, 50 }, 100.0f, blue_v3);
    //draw_line(bitmap_buffer, { 0, 0 }, green_vec, green_v3);
    //draw_line(bitmap_buffer, { 0, 0 }, blue_vec, blue_v3);
    //draw_line(bitmap_buffer, { 0, 0 }, white_vec, white_v3);
}

internal
void drops_update(Game_bitmap_buffer* bitmap_buffer, Game_state* state, Game_input* input) {
    
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
Loaded_bmp debug_load_bmp(char* file_name) {
    Loaded_bmp result = {};
    
    Debug_file_read_result file_result = debug_read_entire_file(file_name);
    
    macro_assert(file_result.bytes_read != 0);
    
    Bitmap_header* header = (Bitmap_header*)file_result.content;
    
    macro_assert(header->Compression == 3);
    
    result.pixels = (u32*)((u8*)file_result.content + header->BitmapOffset);
    
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
    
    i32 shift_alpha = 24 - (i32)scan_alpha.index;
    i32 shift_red   = 16 - (i32)scan_red.index;
    i32 shift_green = 8  - (i32)scan_green.index;
    i32 shift_blue  = 0  - (i32)scan_blue.index;
    
    u32* source_dest = result.pixels;
    for (i32 y = 0; y < header->Height; y++) {
        for (i32 x = 0; x < header->Width; x++) {
            u32 color = *source_dest;
            
            *source_dest++ = (rotate_left(color & mask_alpha, shift_alpha) |
                              rotate_left(color & mask_red,   shift_red  ) |
                              rotate_left(color & mask_green, shift_green) |
                              rotate_left(color & mask_blue,  shift_blue));
        }
    }
    
    return result;
}

internal
void draw_bitmap(Game_bitmap_buffer* bitmap_buffer, Loaded_bmp* bitmap_data, v2 start, v2 align = {0, 0}) {
    
    start -= align;
    
    i32 x0 = round_f32_u32(start.x);
    i32 y0 = round_f32_u32(start.y);
    
    i32 source_offset_x = 0;
    i32 source_offset_y = 0;
    
    if (x0 < 0)
        source_offset_x = -x0;
    
    if (y0 < 0)
        source_offset_y = -y0;
    
    i32 x1 = round_f32_u32(start.x + (f32)bitmap_data->width);
    i32 y1 = round_f32_u32(start.y + (f32)bitmap_data->height);
    
    x0 = clamp_i32(x0, 0, bitmap_buffer->width);  x1 = clamp_i32(x1, 0, bitmap_buffer->width);
    y0 = clamp_i32(y0, 0, bitmap_buffer->height); y1 = clamp_i32(y1, 0, bitmap_buffer->height);
    
    // reading bottom up
    u32* source_row = bitmap_data->pixels + bitmap_data->width * (bitmap_data->height - 1);
    source_row += -bitmap_data->width * source_offset_y + source_offset_x;
    
    u8* dest_row = (u8*)bitmap_buffer->memory + x0 * bitmap_buffer->bytes_per_pixel + y0 * bitmap_buffer->pitch;
    
    for (i32 y = y0; y < y1; y++) {
        u32* dest = (u32*)dest_row; // incoming pixel
        u32* source = source_row;   // backbuffer pixel
        
        for (i32 x = x0; x < x1; x++) {
            f32     a = (f32)((*source >> 24) & 0xff) / 255.0f;
            f32 src_r = (f32)((*source >> 16) & 0xff);
            f32 src_g = (f32)((*source >> 8)  & 0xff);
            f32 src_b = (f32)((*source >> 0)  & 0xff);
            
            f32 dst_r = (f32)((*dest >> 16) & 0xff);
            f32 dst_g = (f32)((*dest >> 8)  & 0xff);
            f32 dst_b = (f32)((*dest >> 0)  & 0xff);
            
            // (1-t)A + tB 
            // alpha (0 - 1);
            // color = dst_color + alpha (src_color - dst_color)
            // color = (1 - alpha)dst_color + alpha * src_color;
            
            // blending techniques: http://ycpcs.github.io/cs470-fall2014/labs/lab09.html
            
            // linear blending
            f32 r = (1.0f - a)*dst_r + a * src_r;
            f32 g = (1.0f - a)*dst_g + a * src_g;
            f32 b = (1.0f - a)*dst_b + a * src_b;
            
            *dest = (((u32)(r + 0.5f) << 16) | 
                     ((u32)(g + 0.5f) << 8)  | 
                     ((u32)(b + 0.5f) << 0));
            
            dest++;
            source++;
        }
        
        dest_row += bitmap_buffer->pitch;
        source_row -= bitmap_data->width;
    }
}

internal
void subpixel_test_udpdate(Game_bitmap_buffer* buffer, Game_state* game_state, Game_input* input, v3 color) {
    
    clear_screen(buffer, color_black_byte);
    
    u8* row = (u8*)buffer->memory + (buffer->width / 2) * buffer->bytes_per_pixel + (buffer->height / 2) * buffer->pitch;
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
Entity* get_entity(Game_state* game_state, u32 index) {
    Entity* result = 0;
    
    if (index > 0 && index < macro_array_count(game_state->entity_list)) {
        result = &game_state->entity_list[index];
    }
    
    return result;
}

internal
u32 add_entity(Game_state* game_state) {
    u32 result = game_state->entity_count++;
    
    macro_assert(game_state->entity_count < macro_array_count(game_state->entity_list));
    
    Entity* entity = &game_state->entity_list[game_state->entity_count++];
    *entity = {};
    
    return result;
}

internal
void init_player(Game_state* game_state, u32 entity_index) {
    Entity* entity = get_entity(game_state, entity_index);
    
    entity->exists = true;
    entity->position.absolute_tile_x = 2;
    entity->position.absolute_tile_y = 4;
    entity->position.absolute_tile_z = 0;
    
    entity->position.offset.x = 0;
    entity->position.offset.y = 0;
    entity->height = .9f;
    entity->width  = .7f;
    
    if (!get_entity(game_state, game_state->following_entity_index)) {
        game_state->following_entity_index = entity_index;
    }
}

void move_player(Game_state* game_state, Entity* entity, v2 player_acceleration_dd, f32 time_delta) {
    
    Tile_map* tile_map = game_state->world->tile_map;
    
    if (player_acceleration_dd.x && player_acceleration_dd.y) {
        player_acceleration_dd *= 0.7071f;
    }
    
    f32 move_speed = 50.0f;
    
    //if (shift_was_pressd)
    //move_speed = 100;
    
    player_acceleration_dd *= move_speed;
    
    player_acceleration_dd += -8.0f * entity->velocity_d;
    
    Tile_map_position old_player_pos = entity->position;
    Tile_map_position new_player_pos = entity->position;
    
    // p' = (1/2) * at^2 + vt + ..
    v2 player_delta =
        0.5f * player_acceleration_dd * square(time_delta) + entity->velocity_d * time_delta;
    // p' = ... + p
    new_player_pos.offset += player_delta;
    
    // v' = at + v
    entity->velocity_d = player_acceleration_dd * time_delta + entity->velocity_d;
    
    new_player_pos = recanonicalize_position(tile_map, new_player_pos);
#if 1
    Tile_map_position player_left = new_player_pos;
    player_left.offset.x -= entity->width/ 2;
    player_left = recanonicalize_position(tile_map, player_left);
    
    Tile_map_position player_right = new_player_pos;
    player_right.offset.x += entity->width / 2;
    player_right = recanonicalize_position(tile_map, player_right);
    
    bool collided = false;
    Tile_map_position collision_pos = {0, 0};
    
    if (!is_world_point_empty(tile_map, new_player_pos)) {
        collision_pos = new_player_pos;
        collided = true;
    }
    
    if (!is_world_point_empty(tile_map, player_left)) {
        collision_pos = player_left;
        collided = true;
    }
    
    if (!is_world_point_empty(tile_map, player_right)) {
        collision_pos = player_right;
        collided = true;
    }
    
    if (collided) {
        // wall
        v2 r = {0, 0};
        if (collision_pos.absolute_tile_x < entity->position.absolute_tile_x) {
            r = v2 {1, 0};
        }
        
        if (collision_pos.absolute_tile_x > entity->position.absolute_tile_x) {
            r = v2 {-1, 0};
        }
        
        if (collision_pos.absolute_tile_y < entity->position.absolute_tile_y) {
            r = v2 {0, 1};
        }
        
        if (collision_pos.absolute_tile_y > entity->position.absolute_tile_y) {
            r = v2 {0, -1};
        }
        
        //v' = v - 2*dot(v,r) * r
        v2 v = entity->velocity_d;
        // game_state->player_velocity_d = v - 2 * inner(v, r) * r; // reflection
        entity->velocity_d = v - 1 * inner(v, r) * r;
    }
    else {
        entity->position = new_player_pos;
    }
#else
    // search in p
    u32 min_tile_x = 0;
    u32 min_tile_y = 0;
    u32 one_past_max_tile_x = 0;
    u32 one_past_max_tile_y = 0;
    u32 abs_tile_z = 0;
    Tile_map_position best_point = entity->position;
    f32 best_distance_squared = length_squared_v2(player_delta);
    
    for (u32 abs_tile_y = 0; abs_tile_y != one_past_max_tile_y; abs_tile_y++) {
        for (u32 abs_tile_x = 0; abs_tile_x != one_past_max_tile_x; abs_tile_x++) {
            
            Tile_map_position test_tile_pos = centered_tile_point(abs_tile_x, abs_tile_y, abs_tile_z);
            u32 tile_value = get_tile_value(tile_map, test_tile_pos);
            
            if (!is_tile_map_empty(tile_value))
                continue;
            
            // assumption: compiler knows these values (min/max_coner) are not using anything from loop so it can pull it out
            v2 min_corner = -0.5f * v2 { tile_map->tile_side_meters, tile_map->tile_side_meters };
            v2 max_corner =  0.5f * v2 { tile_map->tile_side_meters, tile_map->tile_side_meters };
            
            Tile_map_diff rel_new_player_pos = subtract_pos(tile_map, &test_tile_pos, &new_player_pos);
            
            v2 test_point = closest_point_in_rect(min_corner, max_corner, rel_new_player_pos);
            
            test_distance_squared = ;
            
            if (best_distance_squared > test_distance_squared) {
                best_point = ;
                best_distance_squared = ;
            }
            
        }
    }
    
#endif
    if (!are_on_same_tile(&old_player_pos, &entity->position)) {
        u32 tile_value = get_tile_value(tile_map, entity->position);
        
        if (tile_value == 3) { // door up
            entity->position.absolute_tile_z++;
        }
        else if(tile_value == 4) { // door down
            entity->position.absolute_tile_z--;
        }
    }
    
    if (entity->velocity_d.x == 0 && entity->velocity_d.y == 0) {
        // no direction changes
    }
    else if (absolute(entity->velocity_d.x) > absolute(entity->velocity_d.y)) {
        if (entity->velocity_d.x > 0)
            entity->facing_direction = 0;
        else
            entity->facing_direction = 2;
    }
    else {
        if (entity->velocity_d.y > 0)
            entity->facing_direction = 1;
        else
            entity->facing_direction = 3;
    }
    
    
#if 0
    char buffer[256];
    _snprintf_s(buffer, sizeof(buffer), "tile(x,y): %u, %u; relative(x,y): %f, %f\n", 
                entity->position.absolute_tile_x, entity->position.absolute_tile_y,
                entity->position.offset_x, entity->position.offset_y);
    OutputDebugStringA(buffer);
#endif
}

extern "C"
void game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(Game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    Game_state* game_state = (Game_state*)memory->permanent_storage;
    
    // init game state
    if (!memory->is_initialized)
    {
        // reserve null entity for ?
        add_entity(game_state);
        
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
            game_state->background = debug_load_bmp("../data/bg_nebula.bmp");
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
        
        // init memory arenas
        initialize_arena(&game_state->world_arena, 
                         memory->permanent_storage_size - sizeof(Game_state), 
                         (u8*)memory->permanent_storage + sizeof(Game_state));
        
        game_state->world = push_struct(&game_state->world_arena, World);
        World* world = game_state->world;
        world->tile_map = push_struct(&game_state->world_arena, Tile_map);
        
        Tile_map* tile_map = world->tile_map;
        // create tiles
        {
            u32 random_number_index = 0;
            
            // 256 x 256 tile chunks 
            tile_map->chunk_shift = 4;
            tile_map->chunk_mask  = (1 << tile_map->chunk_shift) - 1;
            tile_map->chunk_dimension = (1 << tile_map->chunk_shift);
            
            tile_map->tile_chunk_count_x = 128;
            tile_map->tile_chunk_count_y = 128;
            tile_map->tile_chunk_count_z = 2;
            
            tile_map->tile_side_meters = 1.4f;
            
            u32 tiles_per_width = 17;
            u32 tiles_per_height = 9;
            
            u32 tile_chunk_array_size = tile_map->tile_chunk_count_x * tile_map->tile_chunk_count_y * tile_map->tile_chunk_count_z;
            tile_map->tile_chunks = push_array(&game_state->world_arena, tile_chunk_array_size, Tile_chunk);
            
            u32 screen_x = 0;
            u32 screen_y = 0;
            
            u32 room_goes_horizontal = 1;
            u32 room_has_stairs = 2;
            
            u32 absolute_tile_z = 0;
            
            bool door_north = false;
            bool door_east  = false;
            bool door_south = false;
            bool door_west  = false;
            bool door_up    = false;
            bool door_down  = false;
            
            for (u32 screen_index = 0; screen_index < 100; screen_index++) {
                macro_assert(random_number_index < macro_array_count(random_number_table));
                u32 random_choise;
                if (door_up || door_down) {
                    random_choise = random_number_table[random_number_index++] % 2;
                } else {
                    random_choise = random_number_table[random_number_index++] % 3;
                }
                
                bool created_z_door = false;
                if (random_choise == room_has_stairs) {
                    created_z_door = true;
                    if (absolute_tile_z == 0)
                        door_up = true;
                    else
                        door_down = true;
                }
                else if (random_choise == room_goes_horizontal) {
                    door_east = true;
                }
                else {
                    door_north = true;
                }
                
                for (u32 tile_y = 0; tile_y < tiles_per_height; tile_y++) {
                    for (u32 tile_x = 0; tile_x < tiles_per_width; tile_x++) {
                        u32 absolute_tile_x = screen_x * tiles_per_width + tile_x;
                        u32 absolute_tile_y = screen_y * tiles_per_height + tile_y;
                        
                        u32 tile = 1;
                        
                        if (tile_x == 0 && (!door_west || tile_y != tiles_per_height / 2)) {
                            tile = 2;
                        }
                        
                        if (tile_x == tiles_per_width - 1 && (!door_east || tile_y != tiles_per_height / 2)) {
                            tile = 2;
                        }
                        
                        if (tile_y == 0 && (!door_south || tile_x != tiles_per_width / 2)) {
                            tile = 2;
                        }
                        
                        if (tile_y == tiles_per_height - 1 && (!door_north || tile_x != tiles_per_width / 2)) {
                            tile = 2;
                        }
                        
                        if (tile_y == 6 && tile_x == 6) {
                            if (door_up)
                                tile = 3;
                            
                            if (door_down)
                                tile = 4;
                        }
                        
                        set_tile_value(&game_state->world_arena, tile_map, 
                                       absolute_tile_x, absolute_tile_y, absolute_tile_z, tile);
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
                
                if (random_choise == room_has_stairs) {
                    // toggle between 1 and 0
                    absolute_tile_z = 1 - absolute_tile_z;
                }
                else if (random_choise == room_goes_horizontal) {
                    screen_x++;
                }
                else {
                    screen_y++;
                }
            }
        }
        
        // player's initial tile position
        game_state->camera_pos.absolute_tile_x = 17 / 2;
        game_state->camera_pos.absolute_tile_y = 9 / 2;
        
        memory->is_initialized = true;
    }
    
    World* world = game_state->world;
    Tile_map* tile_map = world->tile_map;
    
    i32 tile_side_pixels = 60;
    f32 meters_to_pixels = tile_side_pixels / tile_map->tile_side_meters;
    
    macro_assert(world);
    macro_assert(tile_map);
    
    // check input and move player
    v2 player_acceleration_dd = {};
    bool shift_was_pressd = false;
    {
        for (i32 controller_index = 0; controller_index < macro_array_count(input->gamepad); controller_index++) {
            auto input_state = get_gamepad(input, controller_index);
            Entity* controlling_entity = get_entity(game_state, game_state->player_index_for_controller[controller_index]);
            
            if (controlling_entity) {
                if (input_state->is_analog) {
                    player_acceleration_dd = v2 {
                        input_state->stick_avg_x,
                        input_state->stick_avg_y
                    };
                } 
                else {
                    // digital
                    if (input_state->up.ended_down) {
                        player_acceleration_dd.y = 1.0f;
                    }
                    if (input_state->down.ended_down) {
                        player_acceleration_dd.y = -1.0f;
                    }
                    if (input_state->left.ended_down) {
                        player_acceleration_dd.x = -1.0f;
                    }
                    if (input_state->right.ended_down) {
                        player_acceleration_dd.x = 1.0f;
                    }
                }
                
                move_player(game_state, controlling_entity, player_acceleration_dd, input->time_delta);
            }
            else {
                if (input_state->start.ended_down) {
                    u32 entity_index = add_entity(game_state);
                    
                    init_player(game_state, entity_index);
                    game_state->player_index_for_controller[controller_index] = entity_index;
                }
            }
        }
    }
    
    // update camera
    {
        Entity* camera_follow_entity = get_entity(game_state, game_state->following_entity_index);
        
        if (camera_follow_entity) {
            game_state->camera_pos.absolute_tile_z = camera_follow_entity->position.absolute_tile_z;
            
            Tile_map_diff diff = subtract_pos(tile_map, &camera_follow_entity->position, &game_state->camera_pos);
            
            if (diff.xy.x > 8.5 * tile_map->tile_side_meters)
                game_state->camera_pos.absolute_tile_x += 17;
            
            if (diff.xy.x < -(8.5f * tile_map->tile_side_meters))
                game_state->camera_pos.absolute_tile_x -= 17;
            
            if (diff.xy.y > 4.5f * tile_map->tile_side_meters)
                game_state->camera_pos.absolute_tile_y += 9;
            
            if (diff.xy.y < -(5.0f * tile_map->tile_side_meters))
                game_state->camera_pos.absolute_tile_y -= 9;
        }
    }
    
    // draw background
    draw_bitmap(bitmap_buffer, &game_state->background, v2 {0, 0});
    
    //clear_screen(bitmap_buffer, color_gray_byte);
    
    f32 screen_center_x = (f32)bitmap_buffer->width / 2;
    f32 screen_center_y = (f32)bitmap_buffer->height / 2;
    
    // render tiles
    {
        v3 color_wall     = { .0f, .5f, 1.0f };
        v3 color_empty    = { .3f, .3f, 1.0f };
        v3 color_occupied = { .9f, .0f, 1.0f };
        // used to draw player from the middle of the tile
        
        for (i32 rel_row = -10; rel_row < 10; rel_row++) {
            for (i32 rel_col = -20; rel_col < 20; rel_col++) {
                u32 col = game_state->camera_pos.absolute_tile_x + rel_col;
                u32 row = game_state->camera_pos.absolute_tile_y + rel_row;
                
                u32 tile_id = get_tile_value(tile_map, col, row, game_state->camera_pos.absolute_tile_z);
                
                if (tile_id > 1) {
                    v3 color = color_empty;
                    
                    if (tile_id == 2)
                        color = color_wall;
                    
                    if (tile_id > 2) // stairs
                        color = gold_v3;
                    
                    if (row == game_state->camera_pos.absolute_tile_y && col == game_state->camera_pos.absolute_tile_x) {
                        color = color_occupied;
                    }
                    
                    v2 half_tile_pixels = { 0.5f * tile_side_pixels, 0.5f * tile_side_pixels };
                    v2 center = { 
                        screen_center_x - meters_to_pixels * game_state->camera_pos.offset.x + (f32)rel_col * tile_side_pixels,
                        screen_center_y + meters_to_pixels * game_state->camera_pos.offset.y - (f32)rel_row * tile_side_pixels 
                    };
                    
                    v2 min = center - half_tile_pixels;
                    v2 max = center + half_tile_pixels;
                    
                    draw_rect(bitmap_buffer, min, max, color);
                }
            }
        }
    }
    
    // draw players
    {
        Entity* entity = game_state->entity_list;
        for (u32 i = 0; i < game_state->entity_count; i++,
             entity++) {
            
            if (!entity->exists)
                continue;
            
            v3 color_player = { .1f, .0f, 1.0f };
            
            Tile_map_diff diff = subtract_pos(tile_map, &entity->position, &game_state->camera_pos);
            
            v2 player_ground_point = { 
                screen_center_x + diff.xy.x * meters_to_pixels,
                screen_center_y - diff.xy.y * meters_to_pixels
            };
            
            v2 player_start = { 
                player_ground_point.x - .5f * entity->width * meters_to_pixels,
                player_ground_point.y - entity->height * meters_to_pixels 
            };
            
            v2 player_end = {
                player_start.x + entity->width * meters_to_pixels, 
                player_start.y + entity->height * meters_to_pixels, 
            };
            
            draw_rect(bitmap_buffer, player_start, player_end, color_player);
            
            Hero_bitmaps* hero_bitmap = &game_state->hero_bitmaps[entity->facing_direction];
            draw_bitmap(bitmap_buffer, &hero_bitmap->hero_body, player_ground_point, hero_bitmap->align);
        }
    }
    
#if 0
    subpixel_test_udpdate(bitmap_buffer, game_state, input, COLOR_GOLD);
#endif
    
#if 0
    drops_update(bitmap_buffer, game_state, input);
#endif
    
#if 0
    vectors_update(bitmap_buffer, game_state, input);
#endif
}

extern "C" 
void game_get_sound_samples(thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer) {
    auto game_state = (Game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400, game_state);
}