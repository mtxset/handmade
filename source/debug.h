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

Debug_record debug_record_list[];

struct Timed_block {
  Debug_record *record;
  u64 start_cycles;
  u32 hit_count;
  
  bool ended = false;
  
  Timed_block(u32 counter, char *file_name, i32 line_number, char *function_name, int hit_count_param = 1) {
    hit_count = hit_count_param;
    
    record = debug_record_list + counter;
    record->file_name = file_name;
    record->line_number = line_number;
    record->function_name = function_name;
    
    start_cycles = __rdtsc();
  }
  
  ~Timed_block() {
    if (ended)
      return;
    
    u64 delta = (__rdtsc() - start_cycles) | ((u64)hit_count << 32);
    atomic_add_u64(&record->hit_count_cycle_count, delta);
  }
  
  void end() {
    ended = true;
    
    u64 delta = (__rdtsc() - start_cycles) | ((u64)hit_count << 32);
    atomic_add_u64(&record->hit_count_cycle_count, delta);
  }
  
};

#endif //DEBUG_H
