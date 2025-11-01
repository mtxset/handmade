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

#include "config.h"

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
i32 
string_len(char* string) {
  i32 result = 0;
  
  // *string dereference value
  // string++ advances poi32er
  // search for null terminator
  while (*string++ != 0) {
    result++;
  }
  
  return result;
}

inline
i16
truncate_u32_i16(u32 value) {
  assert(value <= 65536);
  assert(value >= 0);
  return (u16)value;
}

inline
i16
truncate_i32_i16(i32 value) {
  assert(value <=  32768 - 1);
  assert(value >= -32768);
  return (u16)value;
}

inline
u16
truncate_i32_u16(i32 value) {
  assert(value <= 65536);
  assert(value >= 0);
  return (u16)value;
}

inline
u32 
truncate_u64_u32(u64 value) {
  assert(value <= 0xFFFFFFFF);
  return (u32)value;
}

struct Game_bitmap_buffer {
  // pixels are always 32 bit, memory order BB GG RR XX (padding)
  void* memory;
  i32 width;
  i32 height;
  i32 pitch;
  u32 window_width;
  u32 window_height;
};

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

enum Game_input_mouse_button {
  
  Game_input_mouse_button_left,
  Game_input_mouse_button_middle,
  Game_input_mouse_button_right,
  
  Game_input_mouse_button_count
};

struct Game_input {
  Game_button_state mouse_buttons[3];
  f32 mouse_x, mouse_y, mouse_z; // mouse_z???
  
  // seconds to advance over update
  f32 time_delta;
  
  // 1 - keyboard, other gamepads
  Game_controller_input gamepad[5];
};

inline
bool
was_pressed(Game_button_state state) {
  bool result = (state.half_transition_count > 1 || 
                 (state.half_transition_count == 1) && state.ended_down);
  
  return result;
}

typedef enum Platform_file_type {
  Platform_file_type_asset,
  Platform_file_type_save,
  
  Platform_file_type_count
} Platform_file_type;

typedef struct Platform_file_handle {
  bool no_errors;
  void *platform;
} Platform_file_handle;

typedef struct Platform_file_group {
  u32  file_count;
  void *platform;
} Platform_file_group;

typedef struct Debug_file_read_result {
  u32 bytes_read;
  void* content;
} Debug_file_read_result;

typedef struct Debug_executing_process {
  u64 os_handle;
} Debug_executing_process;

typedef struct Debug_process_state {
  bool started_successfully;
  bool is_running;
  i32 return_code;
} Debug_process_state;

struct Platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(Platform_work_queue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(Platform_work_queue_callback);

typedef void 
Platform_add_entry(Platform_work_queue *queue, Platform_work_queue_callback *callback, void *data);
typedef void 
Platform_complete_all_work(Platform_work_queue *queue);

typedef Platform_file_group
Platform_get_all_files_of_type_begin(Platform_file_type file_type);

typedef void
Platform_get_all_files_of_type_end(Platform_file_group *file_group);

typedef Platform_file_handle
Platform_open_next_file(Platform_file_group *file_group);

typedef void 
Platform_read_data_from_file(Platform_file_handle *src, u64 offset, u64 size, void *dst);

typedef void
Platform_file_error(Platform_file_handle *handle, char *message);

typedef void
Debug_platform_free_file_memory(void *memory);

typedef Debug_file_read_result
Debug_platform_read_entire_file(char *filename);

typedef 
bool
Debug_platform_write_entire_file(char *filename, u32 memory_size, void *memory);

typedef 
Debug_executing_process
Debug_platform_execute_cmd(char *path, char *cmd, char *cmd_line);

typedef
Debug_process_state
Debug_platform_get_process_state(Debug_executing_process process);

#define DEBUG_PLATFORM_GET_PROCESS_STATE(name) debug_process_state name(debug_executing_process Process)

typedef void*
Platform_allocate_memory(sz size);

typedef void
Platform_deallocate_memory(void *memory);

typedef struct Platform_api {
  Platform_add_entry *add_entry;
  Platform_complete_all_work *complete_all_work;
  
  Platform_get_all_files_of_type_begin *get_all_files_of_type_begin;
  Platform_get_all_files_of_type_end   *get_all_files_of_type_end;
  
  Platform_allocate_memory   *allocate_memory;
  Platform_deallocate_memory *deallocate_memory;
  
  Platform_open_next_file      *open_next_file;
  Platform_read_data_from_file *read_data_from_file;
  
  Platform_file_error          *file_error;
  
  Debug_platform_free_file_memory  *debug_free_file_memory;
  Debug_platform_read_entire_file  *debug_read_entire_file;
  Debug_platform_write_entire_file *debug_write_entire_file;
  
  Debug_platform_execute_cmd *debug_execute_cmd;
  Debug_platform_get_process_state *debug_get_process_state;
  
} Platform_api;

typedef struct Game_memory {
  u64 permanent_storage_size;
  void* permanent_storage;
  
  u64 transient_storage_size;
  void* transient_storage;
  
  u64 debug_storage_size;
  void *debug_storage;
  
  Platform_work_queue *high_priority_queue;
  Platform_work_queue *low_priority_queue;
  
  bool executable_reloaded;
  Platform_api platform_api;
  
  u64 cpu_frequency;
  
} Game_memory;

typedef 
void (game_update_render_signature) 
(Game_memory* memory, Game_input* input, Game_bitmap_buffer* bitmap_buffer);

typedef 
void (game_get_sound_samples_signature) 
(Game_memory* memory, Game_sound_buffer* sound_buffer);

inline
Game_controller_input* 
get_gamepad(Game_input* input, i32 input_index) {
  assert(input_index >= 0);
  assert(input_index < array_count(input->gamepad));
  
  return &input->gamepad[input_index];
}

#if INTERNAL
extern struct Game_memory *debug_global_memory;
#endif

#if COMPILER_MSVC

#include <intrin.h>
inline u32 
atomic_compare_exchange_u32(u32 volatile *value, u32 new_value, u32 expected) {
  u32 result = _InterlockedCompareExchange((long volatile*)value, new_value, expected);
  return result;
}

inline
u64
atomic_add_u64(u64 volatile *value, u64 addend) {
  
  u64 result = _InterlockedExchangeAdd64((__int64 volatile *)value, addend);
  
  return result;
}

inline
u64
atomic_exchange_u64(u64 volatile *value, u64 new_value) {
  u64 result = _InterlockedExchange64((__int64 volatile *)value, new_value);
  
  return result;
}

inline 
u32 get_thread_id() {
  u8 *thread_local_storage = (u8*)__readgsqword(0x30);
  u32 thread_id = *(u32*)(thread_local_storage + 0x48);
  
  return thread_id;
}

extern "C" u32 asm_get_thread_id();
extern "C" u32 asm_get_core_id();

#endif

struct Debug_record {
  char *file_name;
  char *block_name;
  
  u32 line_number;
  u32 reserved;
};

enum Debug_event_type {
  Debug_event_type_frame_marker,
  Debug_event_type_begin_block,
  Debug_event_type_end_block
};

struct Thread_id__Core_id {
  u16 thread_id;
  u16 core_id;
};

struct Debug_event {
  u64 clock;
  
  union {
    Thread_id__Core_id thread_id__core_id;
    f32 seconds_elapsed;
  };
  
  u16 debug_record_index;
  
  u8 translation_unit_index;
  u8 type;
};

#define MAX_DEBUG_TRANSLATION_UNITS 3
#define MAX_DEBUG_EVENT_ARRAY_COUNT 8 // max fps saved
#define MAX_DEBUG_EVENT_COUNT       (32*65536)
#define MAX_DEBUG_RECORD_COUNT      (65536)

struct Debug_table {
  
  u32 current_event_array_index;
  u64 volatile event_array_index__event_index;
  u32 event_count[MAX_DEBUG_EVENT_ARRAY_COUNT];
  Debug_event event_list[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
  
  u32 record_count[MAX_DEBUG_TRANSLATION_UNITS];
  Debug_record record_list[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
};

extern Debug_table *global_debug_table;

typedef
Debug_table *(debug_game_frame_end_signature) (Game_memory* memory);

inline
Debug_event*
record_debug_event_common(i32 record_index, Debug_event_type event_type) {
  u64 array_index__event_index = atomic_add_u64(&global_debug_table->event_array_index__event_index, 1);
  
  u32 event_index = array_index__event_index & 0xffffffff;
  
  assert(event_index < MAX_DEBUG_EVENT_COUNT);
  Debug_event *event = global_debug_table->event_list[array_index__event_index >> 32] + event_index;
  
  event->clock = __rdtsc();
  event->debug_record_index = (u16)record_index;
  event->translation_unit_index = TRANSLATION_UNIT_INDEX;
  event->type = (u8)event_type;
  
  return event;
}

inline
void
record_debug_event(i32 record_index, Debug_event_type event_type) {
  
  Debug_event *event = record_debug_event_common(record_index, event_type);
  
  event->thread_id__core_id.thread_id = (u16)asm_get_thread_id();
  event->thread_id__core_id.core_id   = (u16)asm_get_core_id();
}

inline
void
frame_marker(f32 seconds_elapsed_init) {
  i32 counter = __COUNTER__;
  Debug_event *event = record_debug_event_common(counter, Debug_event_type_frame_marker);
  event->seconds_elapsed = seconds_elapsed_init;
  
  Debug_record *record = global_debug_table->record_list[TRANSLATION_UNIT_INDEX] + counter;
  record->file_name = __FILE__;
  record->line_number = __LINE__;
  record->block_name = "frame marker";
}

#if INTERNAL
#define timed_block_raw(block_name, id, ...) \
Timed_block Timed_block##id(__COUNTER__, __FILE__, __LINE__, block_name, ## __VA_ARGS__);

#define timed_block_sub(block_name, id, ...) \
timed_block_raw(block_name, id, ## __VA_ARGS__)

#define timed_block(block_name, ...) timed_block_sub(block_name, __LINE__, ## __VA_ARGS__)
#define timed_function(...) timed_block(__FUNCTION__, __LINE__, ## __VA_ARGS__)

#define timed_block_begin(name)\
i32 counter_##name = __COUNTER__; \
{Debug_record *record = global_debug_table->record_list[TRANSLATION_UNIT_INDEX] + counter_##name; \
record->file_name = __FILE__; \
record->line_number = __LINE__; \
record->block_name = #name; \
record_debug_event(counter_##name, Debug_event_type_begin_block); }\

#define timed_block_end(name) record_debug_event(counter_##name, Debug_event_type_end_block);


#else
#define timed_block(id, ...)
#define timed_function()
#define timed_block_begin(id)
#define timed_block_end(id)
#endif

struct Timed_block {
  i32 counter;
  
  Timed_block(u32 counter_init, char *file_name, i32 line_number, char *block_name, int hit_count_param = 1) {
    counter = counter_init;
    
    Debug_record *record = global_debug_table->record_list[TRANSLATION_UNIT_INDEX] + counter;
    
    record->file_name = file_name;
    record->line_number = line_number;
    record->block_name = block_name;
    
    record_debug_event(counter, Debug_event_type_begin_block);
    
    //begin_block_(counter, file_name, line_number, block_name);
  }
  
  ~Timed_block() {
    record_debug_event(counter, Debug_event_type_end_block);
  }
};

// because it's preprocessing it parses and replaces each __COUNTER__ incremental value
// so by the time it gets here it's increments again, and we compile having ids
// __COUNTER__ will be incremented each time anyone asks for it (per TU)

#endif //PLATFORM_H
