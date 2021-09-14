#include <stdio.h>

#include "types.h"
#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "intrinsics.h"
#include "game.h"

void clear_screen(game_bitmap_buffer* bitmap_buffer, u32 color) {
    auto row = (u8*)bitmap_buffer->memory;
    
    for (i32 y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (u32*)row;
        for (i32 x = 0; x < bitmap_buffer->width; x++) {
            *pixel++ = color;
        }
        row += bitmap_buffer->pitch;
    }
}

void render_255_gradient(game_bitmap_buffer* bitmap_buffer, i32 blue_offset, i32 green_offset) {
    static i32 offset = 1;
    auto row = (u8*)bitmap_buffer->memory;
    
    clear_screen(bitmap_buffer, color_gray);
    
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
    
    static bool go_up = true;
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

static 
void game_output_sound(game_sound_buffer* sound_buffer, i32 tone_hz, game_state* state) {
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

static 
void draw_rect(game_bitmap_buffer* bitmap_buffer, f32 f32_min_x, f32 f32_min_y, f32 f32_max_x, f32 f32_max_y, color_f32 color) {
    
    // 0x alpha    Red      Green    Blue
    // 32 bits
    // 0x 00000000 00000000 00000000 00000000
    u32 color_bits = (round_f32_u32(color.r * 255.0f) << 16 |
                      round_f32_u32(color.g * 255.0f) << 8  |
                      round_f32_u32(color.b * 255.0f) << 0);
    
    i32 min_x = round_f32_i32(f32_min_x);
    i32 max_x = round_f32_i32(f32_max_x);
    i32 min_y = round_f32_i32(f32_min_y);
    i32 max_y = round_f32_i32(f32_max_y);
    
    if (min_x < 0)
        min_x = 0;
    
    if (min_y < 0)
        min_y = 0;
    
    if (max_x > bitmap_buffer->width)
        max_x = bitmap_buffer->width;
    
    if (max_y > bitmap_buffer->height)
        max_y = bitmap_buffer->height;
    
    auto end_buffer = (u8*)bitmap_buffer->memory + bitmap_buffer->pitch * bitmap_buffer->height;
    
    auto row = (u8*)bitmap_buffer->memory + min_x * bitmap_buffer->bytes_per_pixel + min_y * bitmap_buffer->pitch;
    for (i32 y = min_y; y < max_y; y++) {
        auto pixel = (u32*)row;
        for (i32 x = min_x; x < max_x; x++) {
            *pixel++ = color_bits;
        }
        row += bitmap_buffer->pitch;
    }
}

void debug_read_and_write_random_file() {
    auto bitmap_read = debug_read_entire_file(__FILE__);
    if (bitmap_read.content) {
        debug_write_entire_file("temp.cpp", bitmap_read.bytes_read, bitmap_read.content);
        debug_free_file(bitmap_read.content);
    }
}

tile_chunk* get_tile_chunk(world_map_data* world_map, i32 x, i32 y) {
    tile_chunk* result = 0;
    
    if (x >= 0 && x < world_map->tile_chunk_count_x &&
        y >= 0 && y < world_map->tile_chunk_count_y) {
        result = &world_map->tile_chunks[y * world_map->tile_chunk_count_x + x];
    }
    
    return result;
}

u32 get_tile_entry_unchecked(world_map_data* world, tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y) {
    macro_assert(chunk);
    macro_assert(player_tile_x < world->chunk_dimension);
    macro_assert(player_tile_y < world->chunk_dimension);
    
    u32 result;
    
    result = chunk->tiles[player_tile_y * world->chunk_dimension + player_tile_x];
    
    return result;
}

static
bool get_tile_value(world_map_data* world, tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y) {
    u32 result = 0;
    
    if (!chunk)
        return 0;
    
    result = get_tile_entry_unchecked(world, chunk, player_tile_x, player_tile_y);
    
    return result;
}

void recanonicalize_coord(world_map_data* world_map, u32* tile, f32* tile_relative) {
    i32 offset = floor_f32_i32((*tile_relative) / world_map->tile_side_meters);
    *tile += offset;
    *tile_relative -= offset * world_map->tile_side_meters;
    
    macro_assert(*tile_relative >= 0 && *tile_relative <= world_map->tile_side_meters);
}

world_position recanonicalize_position(world_map_data* world_map, world_position pos) {
    
    world_position result = pos;
    
    recanonicalize_coord(world_map, &result.absolute_tile_x, &result.tile_relative_x);
    recanonicalize_coord(world_map, &result.absolute_tile_y, &result.tile_relative_y);
    
    return result;
}

tile_chunk_position get_chunk_position_for(world_map_data* world_data, u32 absolute_tile_x, u32 absolute_tile_y) {
    tile_chunk_position result;
    
    result.tile_chunk_x = absolute_tile_x >> world_data->chunk_shift;
    result.tile_chunk_y = absolute_tile_y >> world_data->chunk_shift;
    
    result.tile_relative_x = absolute_tile_x & world_data->chunk_mask;
    result.tile_relative_y = absolute_tile_y & world_data->chunk_mask;
    
    return result;
}

static
u32 get_tile_value(world_map_data* world_map, u32 tile_abs_x, u32 tile_abs_y) {
    bool result = false;
    
    tile_chunk_position chunk_pos = get_chunk_position_for(world_map, tile_abs_x, tile_abs_y);
    tile_chunk* test_tile_chunk = get_tile_chunk(world_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    result = get_tile_value(world_map, test_tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y);
    
    return result;
}

static
bool is_world_point_empty(world_map_data* world_map, world_position can_pos) {
    bool result = false;
    
    u32 tile_chunk_value = get_tile_value(world_map, can_pos.absolute_tile_x, can_pos.absolute_tile_y);
    
    result = (tile_chunk_value == 0);
    
    return result;
}

extern "C" 
void game_update_render(thread_context* thread, game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    auto state = (game_state*)memory->permanent_storage;
    if (!memory->is_initialized) {
        state->player_pos.absolute_tile_x = 4;
        state->player_pos.absolute_tile_y = 4;
        
        state->player_pos.tile_relative_x = 0;
        state->player_pos.tile_relative_y = 0;
        
        memory->is_initialized = true;
    }
    
    // check input
    f32 player_x_delta = .0f;
    f32 player_y_delta = .0f;
    f32 move_offset = 2.0f;
    {
        for (i32 i = 0; i < macro_array_count(input->gamepad); i++) {
            auto input_state = get_gamepad(input, i);
            
            if (input_state->is_analog) {
                // analog
                if (input_state->move_up.ended_down)    player_y_delta = 1.0f;
                if (input_state->move_down.ended_down)  player_y_delta = -1.0f;
                if (input_state->move_left.ended_down)  player_x_delta = -1.0f;
                if (input_state->move_right.ended_down) player_x_delta = 1.0f;
            } 
            else {
                // digital
                if (input_state->up.ended_down)    player_y_delta = 1.0f;
                if (input_state->down.ended_down)  player_y_delta = -1.0f;
                if (input_state->left.ended_down)  player_x_delta = -1.0f;
                if (input_state->right.ended_down) player_x_delta = 1.0f;
                
                if (input_state->shift.ended_down) move_offset *= 4;
            }
        }
    }
    
    // create tiles
    const i32 cols_or_y = 256;
    const i32 rows_or_x = 256;
    world_map_data world_map;
    {
        u32 tiles[cols_or_y][rows_or_x] = {
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1},
            {1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1},
            {1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
            {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        };
        
        // 256 x 256 tile chunks 
        world_map.chunk_shift = 8;
        world_map.chunk_mask  = (1 << world_map.chunk_shift) - 1;
        world_map.chunk_dimension = 256;
        
        tile_chunk tile_chunk_temp;
        tile_chunk_temp.tiles = (u32*)tiles;
        world_map.tile_chunks = &tile_chunk_temp;
        
        world_map.tile_chunk_count_x = 1;
        world_map.tile_chunk_count_y = 1;
        
        world_map.tile_side_meters = 1.4f;
        world_map.tile_side_pixels = 60;
        world_map.meters_to_pixels = world_map.tile_side_pixels / world_map.tile_side_meters;
    }
    
    f32 player_height = 1.4f;
    f32 player_width  = .5f * player_height;
    
    // move player
    {
        world_position new_player_pos = state->player_pos;
        
        new_player_pos.tile_relative_x += player_x_delta * move_offset * input->time_delta;
        new_player_pos.tile_relative_y += player_y_delta * move_offset * input->time_delta;
        new_player_pos = recanonicalize_position(&world_map, new_player_pos);
        
        world_position player_left = new_player_pos;
        player_left.tile_relative_x -= (player_width / 2);
        player_left = recanonicalize_position(&world_map, player_left);
        
        world_position player_right = new_player_pos;
        player_right.tile_relative_x += (player_width / 2);
        player_right = recanonicalize_position(&world_map, player_right);
        
        if (is_world_point_empty(&world_map, new_player_pos) &&
            is_world_point_empty(&world_map, player_left) &&
            is_world_point_empty(&world_map, player_right)) {
            state->player_pos = new_player_pos;
        }
#if 0
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "tile(x,y): %u, %u; relative(x,y): %f, %f\n", state->player_pos.tile_x, state->player_pos.tile_y, state->player_pos.tile_relative_x, state->player_pos.tile_relative_y);
        OutputDebugStringA(buffer);
#endif
    }
    
    f32 center_x = (f32)bitmap_buffer->width / 2;
    f32 center_y = (f32)bitmap_buffer->height / 2;
    
    // draw tile maps
    {
        color_f32 color_tile     = { .0f, .5f, 1.0f };
        color_f32 color_empty    = { .3f, .3f, 1.0f };
        color_f32 color_occupied = { .9f, .0f, 1.0f };
        
        for (i32 rel_row  = -10; rel_row < 10; rel_row++) {
            for (i32 rel_col = -20; rel_col < 20; rel_col++) {
                u32 col = state->player_pos.absolute_tile_x + rel_col;
                u32 row = state->player_pos.absolute_tile_y + rel_row;
                
                auto tile_id = get_tile_value(&world_map, col, row);
                
                auto color = color_empty;
                
                if (tile_id)
                    color = color_tile;
                
                if (row == state->player_pos.absolute_tile_y && col == state->player_pos.absolute_tile_x)
                    color = color_occupied;
                
                f32 min_x = center_x + (f32)rel_col * world_map.tile_side_pixels;
                f32 min_y = center_y - (f32)rel_row * world_map.tile_side_pixels;
                f32 max_x = min_x + world_map.tile_side_pixels;
                f32 max_y = min_y - world_map.tile_side_pixels;
                
                draw_rect(bitmap_buffer, min_x, max_y, max_x, min_y, color);
            }
        }
    }
    
    // draw player
    {
        color_f32 color_player = { .1f, .0f, 1.0f };
        f32 player_left = center_x + world_map.meters_to_pixels * state->player_pos.tile_relative_x - .5f * player_width * world_map.meters_to_pixels;
        
        f32 player_top = center_y - world_map.meters_to_pixels * state->player_pos.tile_relative_y - player_height * world_map.meters_to_pixels;
        
        draw_rect(bitmap_buffer, 
                  player_left, 
                  player_top, 
                  player_left + player_width * world_map.meters_to_pixels, 
                  player_top + player_height * world_map.meters_to_pixels, 
                  color_player);
    }
}

extern "C" 
void game_get_sound_samples(thread_context* thread, game_memory* memory, game_sound_buffer* sound_buffer) {
    auto state = (game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400,  state);
}