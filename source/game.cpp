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
    
    u32 x0 = round_f32_u32(start_x); u32 x1 = round_f32_u32(end_x);
    u32 y0 = round_f32_u32(start_y); u32 y1 = round_f32_u32(end_y);
    
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

#if 0
#include "pacman.cpp"
#endif
\
static
Loaded_bmp debug_load_bmp(char* file_name) {
    Loaded_bmp result = {};
    
    Debug_file_read_result file_result = debug_read_entire_file(file_name);
    
    macro_assert(file_result.bytes_read != 0);
    
    Bitmap_header* header = (Bitmap_header*)file_result.content;
    
    result.pixels = (u32*)((u8*)file_result.content + header->BitmapOffset);
    
    result.width  = header->Width;
    result.height = header->Height;
#if 0
    u32* source_dest = result.pixels;
    for (i32 y = 0; y < header->Height; y++) {
        for (i32 x = 0; x < header->Width; x++) {
            *source_dest = (*source_dest >> 8) | (*source_dest << 24);
            source_dest++;
        }
    }
#endif
    
    return result;
}

static 
void draw_bitmap(Game_bitmap_buffer* bitmap_buffer, Loaded_bmp* bitmap_data, f32 start_x, f32 start_y) {
    
    i32 x0 = round_f32_u32(start_x); 
    i32 y0 = round_f32_u32(start_y); 
    
    i32 x1 = round_f32_u32(start_x + (f32)bitmap_data->width);
    i32 y1 = round_f32_u32(start_y + (f32)bitmap_data->height);
    
    x0 = clamp_i32(x0, 0, bitmap_buffer->width);  x1 = clamp_i32(x1, 0, bitmap_buffer->width);
    y0 = clamp_i32(y0, 0, bitmap_buffer->height); y1 = clamp_i32(y1, 0, bitmap_buffer->height);
    
    // reading bottom up
    u32* source_row = bitmap_data->pixels + bitmap_data->width * (bitmap_data->height - 1);
    u8* dest_row = (u8*)bitmap_buffer->memory + x0 * bitmap_buffer->bytes_per_pixel + y0 * bitmap_buffer->pitch;
    
    for (i32 y = y0; y < y1; y++) {
        u32* dest = (u32*)dest_row;
        u32* source = source_row;
        for (i32 x = x0; x < x1; x++) {
            *dest++ = *source++;
        }
        dest_row += bitmap_buffer->pitch;
        source_row -= bitmap_data->width;
    }
}

extern "C"
void game_update_render(thread_context* thread, Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(Game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    Game_state* game_state = (Game_state*)memory->permanent_storage;
    
    // init game state
    if (!memory->is_initialized)
    {
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
        
        game_state->background = debug_load_bmp("../data/bg_nebula.bmp");
        game_state->hero = debug_load_bmp("../data/ship.bmp");
        
        
        // init memory arenas
        initialize_arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(Game_state), (u8*)memory->permanent_storage + sizeof(Game_state));
        
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
                        
                        set_tile_value(&game_state->world_arena, tile_map, absolute_tile_x, absolute_tile_y, absolute_tile_z, tile);
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
        game_state->player_pos.absolute_tile_x = 2;
        game_state->player_pos.absolute_tile_y = 4;
        game_state->player_pos.absolute_tile_z = 0;
        
        game_state->player_pos.offset_x = 0;
        game_state->player_pos.offset_y = 0;
        
        memory->is_initialized = true;
    }
    
    World* world = game_state->world;
    Tile_map* tile_map = world->tile_map;
    
    i32 tile_side_pixels = 60;
    f32 meters_to_pixels = tile_side_pixels / tile_map->tile_side_meters;
    
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
                
                if (input_state->shift.ended_down) move_offset *= 10;
            }
        }
    }
    
    f32 player_height = 1.4f;
    f32 player_width  = .5f * player_height;
    
    // move player
    {
        Tile_map_position new_player_pos = game_state->player_pos;
        
        new_player_pos.offset_x += player_x_delta * move_offset * input->time_delta;
        new_player_pos.offset_y += player_y_delta * move_offset * input->time_delta;
        new_player_pos = recanonicalize_position(tile_map, new_player_pos);
        
        Tile_map_position player_left = new_player_pos;
        player_left.offset_x -= (player_width / 2);
        player_left = recanonicalize_position(tile_map, player_left);
        
        Tile_map_position player_right = new_player_pos;
        player_right.offset_x += (player_width / 2);
        player_right = recanonicalize_position(tile_map, player_right);
        
        if (is_world_point_empty(tile_map, new_player_pos) &&
            is_world_point_empty(tile_map, player_left) &&
            is_world_point_empty(tile_map, player_right)) {
            
            if (!are_on_same_tile(&game_state->player_pos, &new_player_pos)) {
                u32 tile_value = get_tile_value(tile_map, new_player_pos);
                // door up
                if (tile_value == 3) {
                    new_player_pos.absolute_tile_z++;
                }
                else if(tile_value == 4) {
                    new_player_pos.absolute_tile_z--;
                }
            }
            
            game_state->player_pos = new_player_pos;
        }
#if 1
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "tile(x,y): %u, %u; relative(x,y): %f, %f\n", game_state->player_pos.absolute_tile_x, game_state->player_pos.absolute_tile_y, game_state->player_pos.offset_x, game_state->player_pos.offset_y);
        OutputDebugStringA(buffer);
#endif
    }
    
    // draw background
    draw_bitmap(bitmap_buffer, &game_state->background, 0, 0);
    
    //clear_screen(bitmap_buffer, color_gray_byte);
    
    f32 screen_center_x = (f32)bitmap_buffer->width / 2;
    f32 screen_center_y = (f32)bitmap_buffer->height / 2;
    
    // draw tile maps
    {
        color_f32 color_wall     = { .0f, .5f, 1.0f };
        color_f32 color_empty    = { .3f, .3f, 1.0f };
        color_f32 color_occupied = { .9f, .0f, 1.0f };
        // used to draw player from the middle of the tile
        f32 half_tile_pixels = 0.5f * tile_side_pixels;
        
        for (i32 rel_row = -100; rel_row < 100; rel_row++) {
            for (i32 rel_col = -200; rel_col < 200; rel_col++) {
                u32 col = game_state->player_pos.absolute_tile_x + rel_col;
                u32 row = game_state->player_pos.absolute_tile_y + rel_row;
                
                u32 tile_id = get_tile_value(tile_map, col, row, game_state->player_pos.absolute_tile_z);
                
                if (tile_id > 1) {
                    color_f32 color = color_empty;
                    
                    if (tile_id == 2)
                        color = color_wall;
                    
                    if (tile_id > 2)
                        color = FRGB_GOLD;
                    
                    if (row == game_state->player_pos.absolute_tile_y && col == game_state->player_pos.absolute_tile_x)
                        color = color_occupied;
                    
                    f32 center_x = screen_center_x - meters_to_pixels * game_state->player_pos.offset_x + (f32)rel_col * tile_side_pixels;
                    f32 center_y = screen_center_y + meters_to_pixels * game_state->player_pos.offset_y - (f32)rel_row * tile_side_pixels;
                    
                    f32 min_x = center_x - half_tile_pixels;
                    f32 min_y = center_y - half_tile_pixels;
                    
                    f32 max_x = center_x + half_tile_pixels;
                    f32 max_y = center_y + half_tile_pixels;
                    
                    draw_rect(bitmap_buffer, min_x, min_y, max_x, max_y, color);
                }
            }
        }
    }
    
    // draw player
    {
        color_f32 color_player = { .1f, .0f, 1.0f };
        f32 player_left = screen_center_x - .5f * player_width * meters_to_pixels;
        f32 player_top = screen_center_y - player_height * meters_to_pixels;
        
        draw_rect(bitmap_buffer,
                  player_left,
                  player_top,
                  player_left + player_width * meters_to_pixels, 
                  player_top + player_height * meters_to_pixels, 
                  color_player);
        draw_bitmap(bitmap_buffer, &game_state->hero, player_left, player_top);
    }
    
}

extern "C" 
void game_get_sound_samples(thread_context* thread, Game_memory* memory, Game_sound_buffer* sound_buffer) {
    auto game_state = (Game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400, game_state);
}