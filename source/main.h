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



extern struct Game_memory* debug_global_memory;
#if INTERNAL
internal
void
debug_end_timer(Debug_cycle_counter_type type, u64 start_cycle_count) {
  debug_global_memory->counter_list[type].cycle_count += __rdtsc() - start_cycle_count;
  debug_global_memory->counter_list[type].hit_count++;
}

internal
void
debug_end_timer(Debug_cycle_counter_type type, u64 start_cycle_count, u32 count) {
  debug_global_memory->counter_list[type].cycle_count += __rdtsc() - start_cycle_count;
  debug_global_memory->counter_list[type].hit_count += count;
}
#endif

#endif //MAIN_H
