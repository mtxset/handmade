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

extern "C" 
void game_update_render(thread_context* thread, game_memory* memory, game_input* input, game_bitmap_buffer* bitmap_buffer) {
    macro_assert(sizeof(game_state) <= memory->permanent_storage_size);
    macro_assert(&input->gamepad[0].back - &input->gamepad[0].buttons[0] == macro_array_count(input->gamepad[0].buttons) - 1); // we need to ensure that we take last element in union
    
    auto state = (game_state*)memory->permanent_storage;
    if (!memory->is_initialized) {
        memory->is_initialized = true;
    }
    
    for (int i = 0; i < macro_array_count(input->gamepad); i++) {
        auto input_state = get_gamepad(input, i);
        
        f32 player_x_delta = .0f;
        f32 player_y_delta = .0f;
        f32 move_offset = 64.0f;
        
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
        }
        
        state->player_x += player_x_delta * move_offset * input->time_delta;
        state->player_y += player_y_delta * move_offset * input->time_delta;
    }
    
    const int cols = 9;
    const int rows = 16;
    u32 tile_map[cols][rows] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1},
        {0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    };
    
    clear_screen(bitmap_buffer, color_gray);
    color_f32 color_tile  = { .0f, .5f, 1.0f};
    color_f32 color_empty = { .3f, .3f, 1.0f};
    
    auto tile_width = 960.f / rows;
    auto tile_height = 600.f / cols;
    
    for (int y = 0; y < cols; y++) {
        for (int x = 0; x < rows; x++) {
            auto tile_id = tile_map[y][x];
            
            f32 min_x = (f32)x * tile_width;
            f32 min_y = (f32)y * tile_height;
            f32 max_x = min_x + tile_width;
            f32 max_y = min_y + tile_height;
            
            auto color = color_empty;
            
            if (tile_id)
                color = color_tile;
            
            draw_rect(bitmap_buffer, min_x, min_y, max_x, max_y, color);
        }
    }
    
    color_f32 color_player = { .1f, .0f, 1.0f };
    f32 player_width  = .7f * tile_width;
    f32 player_height = .7f * tile_height;
    f32 player_left   = state->player_x - .5f * player_width;
    f32 player_top    = state->player_y - player_height;
    draw_rect(bitmap_buffer, player_left, player_top, player_left + player_width, player_top + player_height, color_player);
}

extern "C" 
void game_get_sound_samples(thread_context* thread, game_memory* memory, game_sound_buffer* sound_buffer) {
    auto state = (game_state*)memory->permanent_storage;
    game_output_sound(sound_buffer, 400,  state);
}