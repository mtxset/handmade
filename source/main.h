/* date = July 29th 2021 7:07 pm */

#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "game.h"
#include "platform.h"

struct Win32_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    BITMAPINFO info;
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
    i32 bytes_per_pixel;
};

struct Win32_window_dimensions {
    i32 width;
    i32 height;
};

struct Win32_sound_output {
    i32 samples_per_second;
    i32 latency_sample_count;
    i32 bytes_per_sample;
    u32 buffer_size;
    i32 safety_bytes;
    u32 running_sample_index;
};

struct Win32_debug_time_marker {
    u32 output_play_cursor;
    u32 play_cursor;
    
    u32 output_write_cursor;
    u32 write_cursor;
    
    u32 output_location;
    u32 output_bytes;
    
    u32 expected_play_cursor;
};

struct Win32_game_code {
    HMODULE game_code_dll;
    FILETIME dll_last_write_time;
    
    // because we don't have stubs we need to check for 0 before calling
    game_update_render_signature*     update_and_render;
    game_get_sound_samples_signature* get_sound_samples;
    
    bool valid;
};

struct Win32_state {
    u64 total_memory_size;
    void* game_memory_block;
    
    HANDLE recording_file_handle;
    HANDLE playing_file_handle;
    
    i32 recording_input_index;
    i32 playing_input_index;
    
    void* recording_memory[4];
    
    char exe_file_name[MAX_PATH];
    char* last_slash;
};

inline
void initialize_arena(Memory_arena* arena, size_t size, void* base) {
    arena->size = size;
    arena->base = (u8*)base;
    arena->used = 0;
    arena->temp_count = 0;
}

#define mem_push_struct(arena,type)      (type *)mem_push_size_(arena, sizeof(type))
#define mem_push_array(arena,count,type) (type *)mem_push_size_(arena, (count) * sizeof(type))
#define mem_push_size(arena,size)       mem_push_size_(arena, size)

#define mem_zero_struct(instance) mem_zero_size_(sizeof(instance), &(instance))

inline
Temp_memory
begin_temp_memory(Memory_arena* arena) {
    Temp_memory result;
    
    result.arena = arena;
    result.used = arena->used;
    arena->temp_count++;
    
    return result;
}

inline
void
end_temp_memory(Temp_memory temp_memory) {
    Memory_arena* arena = temp_memory.arena;
    macro_assert(arena->used >= temp_memory.used);
    arena->used = temp_memory.used;
    macro_assert(arena->temp_count > 0);
    arena->temp_count--;
}

inline
void
check_arena(Memory_arena* arena) {
    macro_assert(arena->temp_count == 0);
}

inline
void* 
mem_push_size_(Memory_arena* arena, size_t size) {
    macro_assert((arena->used + size) <= arena->size);
    
    void* result = arena->base + arena->used;
    arena->used += size;
    
    return result;
}

inline
void 
mem_zero_size_(size_t size, void* pointer) {
    u8* byte = (u8*)pointer;
    while (size--)
        *byte++ = 0;
}

#endif //MAIN_H
