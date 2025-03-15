/* date = March 1st 2025 11:42 pm */

#ifndef DEBUG_H
#define DEBUG_H

#if (!COMPILER_MSVC && !COMPILER_LLVM)
assert(!"not supported")
#endif

#if INTERNAL
#define timed_block_raw(id, ...) \
Timed_block Timed_block##id(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__);

#define timed_block_sub(id, ...) \
timed_block_raw(id, ## __VA_ARGS__)

#define timed_block(...) timed_block_sub(__LINE__, ## __VA_ARGS__)

#define stop_timed_block(id) Timed_block##id.end()
#else
#define timed_block(id)
#endif

// __COUNTER__ will be incremented each time anyone asks for it (per TU)

struct Debug_record {
  char *file_name;
  char *function_name;
  
  u32 line_number;
  u32 reserved;
  
  u64 hit_count_cycle_count;
};

enum Debug_event_type {
  Debug_event_type_begin_block,
  Debug_event_type_end_block
};

struct Debug_event {
  u64 clock;
  u16 thread_index;
  u16 core_index;
  u16 debug_record_index;
  u8 debug_record_array_index;
  u8 type;
};

Debug_record debug_record_list[];

#define MAX_DEBUG_EVENT_COUNT (32*65536)
extern u64 global_debug_event_array_index_debug_event_index;
extern Debug_event global_debug_event_array[2][MAX_DEBUG_EVENT_COUNT];

inline
void
record_debug_event(i32 record_index, Debug_event_type event_type) {
  u64 array_index__event_index = atomic_add_u64(&global_debug_event_array_index_debug_event_index, 1);
  
  u32 event_index = array_index__event_index & 0xffffffff;
  assert(event_index < MAX_DEBUG_EVENT_COUNT);
  Debug_event *event = global_debug_event_array[array_index__event_index >> 32] + event_index;
  
  event->clock = __rdtsc();
  event->thread_index = 0;
  event->core_index = 0;
  event->debug_record_index = (u16)record_index;
  event->debug_record_array_index = debug_record_array_index_const;
  event->type = (u8)event_type;
}

struct Timed_block {
  Debug_record *record;
  u64 start_cycles;
  u32 hit_count;
  i32 counter;
  
  Timed_block(u32 counter_init, char *file_name, i32 line_number, char *function_name, int hit_count_param = 1) {
    counter = counter_init;
    hit_count = hit_count_param;
    
    record = debug_record_list + counter;
    record->file_name = file_name;
    record->line_number = line_number;
    record->function_name = function_name;
    
    start_cycles = __rdtsc();
    
    record_debug_event(counter, Debug_event_type_begin_block);
  }
  
  ~Timed_block() {
    
    u64 delta = (__rdtsc() - start_cycles) | ((u64)hit_count << 32);
    atomic_add_u64(&record->hit_count_cycle_count, delta);
    
    record_debug_event(counter, Debug_event_type_end_block);
  }
};

struct Debug_counter_snapshot {
  u32 hit_count;
  u64 cycle_count;
};

#define DEBUG_SNAPSHOT_COUNT 120
struct Debug_counter_state {
  char *file_name;
  char *function_name;
  
  u32 line_number;
  
  Debug_counter_snapshot snapshot_list[DEBUG_SNAPSHOT_COUNT];
};

struct Debug_state {
  u32 snapshot_index;
  u32 counter_count;
  Debug_counter_state counter_state_list[512];
  Debug_frame_end_info frame_end_info_list[DEBUG_SNAPSHOT_COUNT];
};


// because it's preprocessing it parses and replaces each __COUNTER__ incremental value
// so by the time it gets here it's increments again, and we compile having ids

struct Render_group;
struct Game_asset_list;
static Render_group *debug_render_group;

static void debug_reset(Game_asset_list *asset_list, u32 width, u32 height);
static void debug_overlay(Game_memory *memory);

#endif //DEBUG_H
