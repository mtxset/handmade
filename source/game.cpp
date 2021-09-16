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

void clear_screen(Game_bitmap_buffer* bitmap_buffer, u32 color) {
    // 8 bit pointer to the beginning of the memory
    // 8 bit so we have arithemtic by 1 byte
    auto row = (u8*)bitmap_buffer->memory;
    for (int y = 0; y < bitmap_buffer->height; y++) {
        // 32 bits so we move by 4 bytes cuz aa rr gg bb
        auto pixel = (u32*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
            *pixel++ = color;
        }
        row += bitmap_buffer->pitch;
    }
}

void render_255_gradient(Game_bitmap_buffer* bitmap_buffer, i32 blue_offset, i32 green_offset) {
    static i32 offset = 1;
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

static
void draw_rect(Game_bitmap_buffer* buffer, f32 start_x, f32 start_y, f32 end_x, f32 end_y, color_f32 color) {
    u32 color_hex = 0;
    
    color_hex = (round_f32_u32(color.r * 255.0f) << 16 | 
                 round_f32_u32(color.g * 255.0f) << 8  |
                 round_f32_u32(color.b * 255.0f) << 0);
    
    auto x0 = round_f32_u32(start_x); auto x1 = round_f32_u32(end_x);
    auto y0 = round_f32_u32(start_y); auto y1 = round_f32_u32(end_y);
    
    x0 = clamp_i32(x0, 0, buffer->width);  x1 = clamp_i32(x1, 0, buffer->width);
    y0 = clamp_i32(y0, 0, buffer->height); y1 = clamp_i32(y1, 0, buffer->height);
    
    auto row = (u8*)buffer->memory + x0 * buffer->bytes_per_pixel + y0 * buffer->pitch;
    
    for (u32 y = y0; y < y1; y++) {
        auto pixel = (u32*)row;
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

static
void drops_update(Game_bitmap_buffer* bitmap_buffer, Game_state* state, Game_input* input) {
    
    // draw mouse pointer
    f32 mouse_draw_offset = 10.0f; // offset from windows layer
    color_f32 color_white = { 1.0f, 1.0f, 1.0f };
    {
        f32 mouse_x = (f32)input->mouse_x - mouse_draw_offset;
        f32 mouse_y = (f32)input->mouse_y - mouse_draw_offset;
        
        draw_rect(bitmap_buffer, mouse_x, mouse_y, mouse_x + mouse_draw_offset, mouse_y + mouse_draw_offset, color_white);
        
        if (input->mouse_buttons[0].ended_down) {
            i32 index = state->drop_index++;
            auto drop = &state->drops[index];
            drop->active = true;
            drop-> a = 0;
            drop->pos_x = mouse_x;
            drop->pos_y = mouse_y;
            
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
            drop->pos_y += 3.0f * input->time_delta + drop->a;
            
            if (drop->pos_y >= bitmap_buffer->height) {
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
            
            draw_rect(bitmap_buffer, drop->pos_x, drop->pos_y, drop->pos_x + mouse_draw_offset, drop->pos_y + mouse_draw_offset, color_white);
        }
    }
}
static
void initialize_arena(Memory_arena* arena, size_t size, u8* base) {
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

#define push_struct(arena, type)       (type *)push_size_(arena, sizeof(type))
#define push_array(arena, count, type) (type *)push_size_(arena, (count) * sizeof(type))
void* push_size_(Memory_arena* arena, size_t size) {
    macro_assert(arena->used + size <= arena->size);
    
    void* result = arena->base + arena->used;
    arena->used += size;
    
    return result;
}

extern "C"
void game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(Game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    // init game state
    auto game_state = (Game_state*)memory->permanent_storage;
    {
        if (!memory->is_initialized) {
            
            // init memory arenas
            initialize_arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(game_state), (u8*)memory->permanent_storage + sizeof(Game_state));
            
            game_state->world = push_struct(&game_state->world_arena, World);
            World* world = game_state->world;
            world->tile_map = push_struct(&game_state->world_arena, Tile_map);
            
            Tile_map* tile_map = world->tile_map;
            // create tiles
            {
                // 256 x 256 tile chunks 
                tile_map->chunk_shift = 4;
                tile_map->chunk_mask  = (1 << tile_map->chunk_shift) - 1;
                tile_map->chunk_dimension = (1 << tile_map->chunk_shift);
                
                tile_map->tile_chunk_count_x = 128;
                tile_map->tile_chunk_count_y = 128;
                
                u32 tile_chunk_array_size = tile_map->tile_chunk_count_x * tile_map->tile_chunk_count_y;
                tile_map->tile_chunks = push_array(&game_state->world_arena, tile_chunk_array_size, Tile_chunk);
                
                u32 array_size = tile_map->chunk_dimension * tile_map->chunk_dimension;
                for (u32 y = 0; y < tile_map->tile_chunk_count_y; y++) {
                    for (u32 x = 0; x < tile_map->tile_chunk_count_x; x++) {
                        tile_map->tile_chunks[y * tile_map->tile_chunk_count_x + x].tiles = push_array(&game_state->world_arena, array_size, u32);
                    }
                }
                
                tile_map->tile_side_meters = 1.4f;
                tile_map->tile_side_pixels = 60;
                tile_map->meters_to_pixels = tile_map->tile_side_pixels / tile_map->tile_side_meters;
                
                u32 tiles_per_width = 17;
                u32 tiles_per_height = 9;
                for (u32 screen_y = 0; screen_y < 32; screen_y++) {
                    for (u32 screen_x = 0; screen_x < 32; screen_x++) {
                        for (u32 tile_y = 0; tile_y < tiles_per_height; tile_y++) {
                            for (u32 tile_x = 0; tile_x < tiles_per_width; tile_x++) {
                                u32 abs_tile_x = screen_x * tiles_per_width + tile_x;
                                u32 abs_tile_y = screen_y * tiles_per_height + tile_y;
                                
                                auto tile = (tile_x == tile_y);
                                
                                set_tile_value(&game_state->world_arena, tile_map, abs_tile_x, abs_tile_y, tile);
                            }
                        }
                    }
                }
            }
            
            // player's initial tile position
            game_state->player_pos.absolute_tile_x = 1;
            game_state->player_pos.absolute_tile_y = 4;
            
            game_state->player_pos.tile_relative_x = 0;
            game_state->player_pos.tile_relative_y = 0;
            
            
            memory->is_initialized = true;
        }
    }
    
    World* world = game_state->world;
    Tile_map* tile_map = world->tile_map;
    
    macro_assert(world);
    macro_assert(tile_map);
    
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
    
    f32 player_height = 1.4f;
    f32 player_width  = .5f * player_height;
    
    // move player
    {
        Tile_map_position new_player_pos = game_state->player_pos;
        
        new_player_pos.tile_relative_x += player_x_delta * move_offset * input->time_delta;
        new_player_pos.tile_relative_y += player_y_delta * move_offset * input->time_delta;
        new_player_pos = recanonicalize_position(tile_map, new_player_pos);
        
        Tile_map_position player_left = new_player_pos;
        player_left.tile_relative_x -= (player_width / 2);
        player_left = recanonicalize_position(tile_map, player_left);
        
        Tile_map_position player_right = new_player_pos;
        player_right.tile_relative_x += (player_width / 2);
        player_right = recanonicalize_position(tile_map, player_right);
        
        if (is_world_point_empty(tile_map, new_player_pos) &&
            is_world_point_empty(tile_map, player_left) &&
            is_world_point_empty(tile_map, player_right)) {
            game_state->player_pos = new_player_pos;
        }
#if 0
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "tile(x,y): %u, %u; relative(x,y): %f, %f\n", game_state->player_pos.absolute_tile_x, game_state->player_pos.absolute_tile_y, game_state->player_pos.tile_relative_x, game_state->player_pos.tile_relative_y);
        OutputDebugStringA(buffer);
#endif
    }
    
    f32 screen_center_x = (f32)bitmap_buffer->width / 2;
    f32 screen_center_y = (f32)bitmap_buffer->height / 2;
    
    // draw tile maps
    {
        color_f32 color_tile     = { .0f, .5f, 1.0f };
        color_f32 color_empty    = { .3f, .3f, 1.0f };
        color_f32 color_occupied = { .9f, .0f, 1.0f };
        // used to draw player from the middle of the tile
        f32 half_tile_pixels = 0.5f * tile_map->tile_side_pixels;
        
        for (i32 rel_row  = -10; rel_row < 10; rel_row++) {
            for (i32 rel_col = -20; rel_col < 20; rel_col++) {
                u32 col = game_state->player_pos.absolute_tile_x + rel_col;
                u32 row = game_state->player_pos.absolute_tile_y + rel_row;
                
                auto tile_id = get_tile_value(tile_map, col, row);
                
                auto color = color_empty;
                
                if (tile_id)
                    color = color_tile;
                
                if (row == game_state->player_pos.absolute_tile_y && col == game_state->player_pos.absolute_tile_x)
                    color = color_occupied;
                
                f32 center_x = screen_center_x - tile_map->meters_to_pixels * game_state->player_pos.tile_relative_x + (f32)rel_col * tile_map->tile_side_pixels;
                f32 center_y = screen_center_y + tile_map->meters_to_pixels * game_state->player_pos.tile_relative_y - (f32)rel_row * tile_map->tile_side_pixels;
                
                f32 min_x = center_x - half_tile_pixels;
                f32 min_y = center_y - half_tile_pixels;
                
                f32 max_x = center_x + half_tile_pixels;
                f32 max_y = center_y + half_tile_pixels;
                
                draw_rect(bitmap_buffer, min_x, min_y, max_x, max_y, color);
            }
        }
    }
    
    // draw player
    {
        color_f32 color_player = { .1f, .0f, 1.0f };
        f32 player_left = screen_center_x - .5f * player_width * tile_map->meters_to_pixels;
        f32 player_top = screen_center_y - player_height * tile_map->meters_to_pixels;
        
        draw_rect(bitmap_buffer,
                  player_left,
                  player_top,
                  player_left + player_width * tile_map->meters_to_pixels, 
                  player_top + player_height * tile_map->meters_to_pixels, 
                  color_player);
    }
    
    //drops_update(bitmap_buffer, game_state, input);
}

extern "C" 
void game_get_sound_samples(thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer) {
    auto game_state = (Game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400, game_state);
}