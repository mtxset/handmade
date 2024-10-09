/* date = September 20th 2021 7:12 pm */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM

#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#endif //!COMPILER_MSVC && !COMPILER_LLVM

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

#define BITMAP_BYTES_PER_PIXEL 4

global_var f32 MAX_F32 = 3.402823466e+38F;
global_var f32 MIN_F32 = 1.175494351e-38F;

global_var const float PI = 3.14159265358979323846f;
global_var const float TAU = 6.28318530717958647692f;;

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if DEBUG
#define assert(expr) if (!(expr)) {*(i32*)0 = 0;}
#else
#define assert(expr)
#endif 

#define kilobytes(value) (value) * 1024ull
#define megabytes(value) (kilobytes(value) * 1024ull)
#define gigabytes(value) (megabytes(value) * 1024ull)
#define terabytes(value) (gigabytes(value) * 1024ull)
#define array_count(array) (sizeof(array) / sizeof((array)[0])) // array is in parenthesis because we can pass x + y and we want to have (x + y)[0]

#define align_04(value) ((value +  3) &  ~3)
#define align_08(value) ((value +  7) &  ~7)
#define align_16(value) ((value + 15) & ~15)

inline
u32 
truncate_u64_u32(u64 value) {
  assert(value <= 0xFFFFFFFF);
  return (u32)value;
}

#if INTERNAL
enum Debug_cycle_counter_type {
  Debug_cycle_counter_type_game_update_render,      // 0
  Debug_cycle_counter_type_render_group_to_output,  // 1
  Debug_cycle_counter_type_render_draw_rect_slow,   // 2
  Debug_cycle_counter_type_process_pixel,           // 3
  Debug_cycle_counter_type_render_draw_rect_quak,   // 4
  Debug_cycle_counter_count
};

struct Game_bitmap_buffer {
  // pixels are always 32 bit, memory order BB GG RR XX (padding)
  void* memory;
  i32 width;
  i32 height;
  i32 pitch;
  u32 window_width;
  u32 window_height;
};

typedef struct Debug_cycle_counter {
  u64 cycle_count;
  u32 hit_count;
} Debug_cycle_counter;
#endif

struct Game_sound_buffer {
  i32 sample_count;
  i32 samples_per_second;
  i16* samples;
};

struct Game_button_state {
  i32 half_transition_count;
  bool ended_down;
};

struct Game_controller_input {
  bool is_connected;
  bool is_analog;
  f32 stick_avg_x;
  f32 stick_avg_y;
  
  union {
    Game_button_state buttons[16];
    struct {
      Game_button_state action_up;
      Game_button_state action_down;
      Game_button_state action_left;
      Game_button_state action_right;
      
      Game_button_state up;
      Game_button_state down;
      Game_button_state left;
      Game_button_state right;
      
      Game_button_state cross_or_a;
      Game_button_state circle_or_b;
      Game_button_state triangle_or_y;
      Game_button_state box_or_x;
      
      Game_button_state shift;
      Game_button_state action;
      
      Game_button_state start;
      // if assert fails increase buttons[x] by amount of new buttons, so total matches count of buttons in this struct
      // back button has to be last entry cuz there is macro which relies on that
      // file: game.cpp function: game_update_render
      Game_button_state back;
    };
  };
};

struct Game_input {
  Game_button_state mouse_buttons[3];
  i32 mouse_x, mouse_y;
  
  // seconds to advance over update
  f32 time_delta;
  
  // 1 - keyboard, other gamepads
  Game_controller_input gamepad[5];
  
  bool executable_reloaded;
};

typedef struct Platform_file_handle {
  bool no_errors;
} Platform_file_handle;

typedef struct Platform_file_group {
  u32  file_count;
  void *data;
} Platform_file_group;

typedef struct Debug_file_read_result {
  u32 bytes_read;
  void* content;
} Debug_file_read_result;

struct Platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(Platform_work_queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(Platform_work_queue_callback);

typedef void 
Platform_add_entry(Platform_work_queue *queue, Platform_work_queue_callback *callback, void *data);
typedef void 
Platform_complete_all_work(Platform_work_queue *queue);

typedef Platform_file_group*
Platform_get_all_files_of_type_begin(char *type);

typedef void
Platform_get_all_files_of_type_end(Platform_file_group *file_group);

typedef Platform_file_handle*
Platform_open_file(Platform_file_group *file_group, u32 file_index);

typedef void 
Platform_read_data_from_file(Platform_file_handle *src, u64 offset, u64 size, void *dst);

typedef void
Platform_file_error(Platform_file_handle *handle, char *message);

typedef void
Debug_platform_free_file_memory(void *memory);

typedef Debug_file_read_result
Debug_platform_read_entire_file(char *filename);

typedef bool
Debug_platform_write_entire_file(char *filename, u32 memory_size, void *memory);



typedef struct Platform_api {
  Platform_add_entry *add_entry;
  Platform_complete_all_work *complete_all_work;
  
  Platform_get_all_files_of_type_begin *get_all_files_of_type_begin;
  Platform_get_all_files_of_type_end   *get_all_files_of_type_end;
  
  Platform_open_file                   *open_file;
  Platform_read_data_from_file         *read_data_from_file;
  
  Platform_file_error                  *file_error;
  
  Debug_platform_free_file_memory  *debug_free_file_memory;
  Debug_platform_read_entire_file  *debug_read_entire_file;
  Debug_platform_write_entire_file *debug_write_entire_file;
  
} Platform_api;

typedef struct Game_memory {
  u64 permanent_storage_size;
  void* permanent_storage;
  
  u64 transient_storage_size;
  void* transient_storage;
  
  Platform_work_queue *high_priority_queue;
  Platform_work_queue *low_priority_queue;
  
  Platform_api platform_api;
  
#if INTERNAL
  Debug_cycle_counter counter_list[Debug_cycle_counter_count];
#endif
} Game_memory;

typedef 
void (game_update_render_signature) (Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) (Game_memory* memory, Game_sound_buffer* sound_buffer);

inline
Game_controller_input* 
get_gamepad(Game_input* input, i32 input_index) {
  assert(input_index >= 0);
  assert(input_index < array_count(input->gamepad));
  
  return &input->gamepad[input_index];
}

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

#endif //PLATFORM_H
