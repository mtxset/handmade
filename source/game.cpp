#include "file_io.h"
#include "file_io.cpp"
#include "utils.h"
#include "utils.cpp"
#include "game.h"

void clear_screen(game_bitmap_buffer* bitmap_buffer, u32 color) {
    auto row = (u8*)bitmap_buffer->memory;
    
    for (int y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (u32*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
            *pixel++ = color;
        }
        row += bitmap_buffer->pitch;
    }
}

void render_255_gradient(game_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {
    static int offset = 1;
    auto row = (u8*)bitmap_buffer->memory;
    
    clear_screen(bitmap_buffer, color_gray);
    
    for (int y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (u32*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
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
    int step = 1;
    
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
void game_output_sound(game_sound_buffer* sound_buffer, int tone_hz, game_state* state) {
    i16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    auto sample_out = sound_buffer->samples;
    
#if 1
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
        i16 sample_value = 0;
        
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
    }
#else
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
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
    
    int min_x = round_f32_i32(f32_min_x);
    int max_x = round_f32_i32(f32_max_x);
    int min_y = round_f32_i32(f32_min_y);
    int max_y = round_f32_i32(f32_max_y);
    
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
    for (int y = min_y; y < max_y; y++) {
        auto pixel = (u32*)row;
        for (int x = min_x; x < max_x; x++) {
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

inline
tile_map_data* get_map_entry(world_map_data* world_map, int x, int y) {
    tile_map_data* result = 0;
    
    if (x >= 0 && x < world_map->rows_or_x &&
        y >= 0 && y < world_map->cols_or_y) {
        result = &world_map->tile_maps[y * world_map->rows_or_x + x];
    }
    
    return result;
}

inline
u32 get_tile_entry_unchecked(tile_map_data* tile_map, int x, int y) {
    u32 result;
    
    result = tile_map->map[y * tile_map->rows_or_x + x];
    
    return result;
}

static
bool is_world_point_empty(world_map_data* world_map, i32 x, i32 y) {
    bool result = false;
    
    auto tile_map = get_map_entry(world_map, x, y);
    
    if (!tile_map)
        return result;
    
    int player_tile_x = truncate_f32_i32(x / tile_map->one_tile_width);
    int player_tile_y = truncate_f32_i32(y / tile_map->one_tile_height);
    
    if (player_tile_x >= 0 && player_tile_x < tile_map->rows_or_x &&
        player_tile_y >= 0 && player_tile_y < tile_map->cols_or_y) {
        auto tile_map_entry = get_tile_entry_unchecked(tile_map, player_tile_x, player_tile_y);
        
        result = tile_map_entry == 0;
    }
    
    return result;
}

static
bool is_tile_empty(tile_map_data* tile_map, f32 x, f32 y) {
    bool result = false;
    
    int player_tile_x = truncate_f32_i32(x / tile_map->one_tile_width);
    int player_tile_y = truncate_f32_i32(y / tile_map->one_tile_height);
    
    if (player_tile_x >= 0 && player_tile_x < tile_map->rows_or_x &&
        player_tile_y >= 0 && player_tile_y < tile_map->cols_or_y) {
        auto tile_map_entry = get_tile_entry_unchecked(tile_map, player_tile_x, player_tile_y);
        
        result = tile_map_entry == 0;
    }
    
    return result;
}

extern "C" 
void game_update_render(thread_context* thread, game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    auto state = (game_state*)memory->permanent_storage;
    if (!memory->is_initialized) {
        state->player_x = 200;
        state->player_y = 200;
        memory->is_initialized = true;
    }
    
    f32 player_x_delta = .0f;
    f32 player_y_delta = .0f;
    f32 move_offset = 64.0f;
    
    // check input
    {
        for (int i = 0; i < macro_array_count(input->gamepad); i++) {
            auto input_state = get_gamepad(input, i);
            
            if (input_state->is_analog) {
                // TODO: fix so y in analog and digital is same (-1 or 1) it is consistent with keyboard and check dpad
                if (input_state->move_up.ended_down)    player_y_delta = 1.0f;
                if (input_state->move_down.ended_down)  player_y_delta = -1.0f;
                if (input_state->move_left.ended_down)  player_x_delta = -1.0f;
                if (input_state->move_right.ended_down) player_x_delta = 1.0f;
            } 
            else {
                // digital
                if (input_state->up.ended_down)    player_y_delta = -1.0f;
                if (input_state->down.ended_down)  player_y_delta = 1.0f;
                if (input_state->left.ended_down)  player_x_delta = -1.0f;
                if (input_state->right.ended_down) player_x_delta = 1.0f;
                if (input_state->shift.ended_down) move_offset *= 2;
            }
        }
    }
    
    const int cols_or_y = 9;
    const int rows_or_x = 16;
    
    u32 tiles_00[cols_or_y][rows_or_x] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1},
        {0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };
    
    u32 tiles_01[cols_or_y][rows_or_x] = {
        {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };
    
    u32 tiles_10[cols_or_y][rows_or_x] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1},
        {0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    };
    
    u32 tiles_11[cols_or_y][rows_or_x] = {
        {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };
    
    color_f32 color_tile  = { .0f, .5f, 1.0f};
    color_f32 color_empty = { .3f, .3f, 1.0f};
    
    const int map_cols_or_y = 2;
    const int map_rows_or_x = 2;
    
    tile_map_data tile_maps[map_cols_or_y][map_rows_or_x];
    tile_maps[0][0].cols_or_y       = cols_or_y;
    tile_maps[0][0].rows_or_x       = rows_or_x;
    tile_maps[0][0].one_tile_width  = 960.f / rows_or_x;
    tile_maps[0][0].one_tile_height = 600.f / cols_or_y;
    tile_maps[0][0].map             = (u32*)tiles_00;
    
    tile_maps[0][1] = tile_maps[0][0];
    tile_maps[0][1].map = (u32*)tiles_01;
    
    tile_maps[1][0] = tile_maps[0][0];
    tile_maps[1][0].map = (u32*)tiles_10;
    
    tile_maps[1][1] = tile_maps[0][0];
    tile_maps[1][1].map = (u32*)tiles_11;
    
    f32 player_width  = .7f * tile_maps[0][0].one_tile_width;
    f32 player_height = .7f * tile_maps[0][0].one_tile_height;
    
    world_map_data world_map;
    world_map.cols_or_y = map_cols_or_y;
    world_map.rows_or_x = map_rows_or_x;
    world_map.tile_maps = (tile_map_data*)tile_maps;
    
    auto current_tile_map = &tile_maps[0][0];
    
    // draw tile map 
    {
        for (int y = 0; y < cols_or_y; y++) {
            for (int x = 0; x < rows_or_x; x++) {
                auto tile_id = get_tile_entry_unchecked(current_tile_map, x, y);
                
                f32 min_x = (f32)x * current_tile_map->one_tile_width;
                f32 min_y = (f32)y * current_tile_map->one_tile_height;
                f32 max_x = min_x + current_tile_map->one_tile_width;
                f32 max_y = min_y + current_tile_map->one_tile_height;
                
                auto color = color_empty;
                
                if (tile_id)
                    color = color_tile;
                
                draw_rect(bitmap_buffer, min_x, min_y, max_x, max_y, color);
            }
        }
    }
    
    // move player 
    {
        f32 new_player_x = state->player_x + player_x_delta * move_offset * input->time_delta;
        f32 new_player_y = state->player_y + player_y_delta * move_offset * input->time_delta;
        
        if (is_tile_empty(current_tile_map, new_player_x + (player_width / 2), new_player_y) &&
            is_tile_empty(current_tile_map, new_player_x - (player_width / 2), new_player_y)) {
            state->player_x = new_player_x;
            state->player_y = new_player_y;
        }
    }
    
    // draw player
    {
        color_f32 color_player = { .1f, .0f, 1.0f };
        f32 player_left   = state->player_x - .5f * player_width;
        f32 player_top    = state->player_y - player_height;
        draw_rect(bitmap_buffer, player_left, player_top, player_left + player_width, player_top + player_height, color_player);
    }
}

extern "C" 
void game_get_sound_samples(thread_context* thread, game_memory* memory, game_sound_buffer* sound_buffer) {
    auto state = (game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400,  state);
}