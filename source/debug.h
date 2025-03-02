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
  u64  cycle_count;
  
  char *file_name;
  char *function_name;
  u32 line_number;
  
  u32 hit_count;
};

Debug_record debug_record_list[];

struct Timed_block {
  Debug_record *record;
  
  bool ended = false;
  
  Timed_block(u32 counter, char *file_name, i32 line_number, char *function_name, int hit_count = 1) {
    record = debug_record_list + counter;
    record->file_name = file_name;
    record->line_number = line_number;
    record->function_name = function_name;
    record->cycle_count -= __rdtsc(); // ??
    record->hit_count += hit_count;
  }
  
  ~Timed_block() {
    if (ended)
      return;
    
    record->cycle_count += __rdtsc();
  }
  
  void end() {
    ended = true;
    
    record->cycle_count += __rdtsc();
  }
  
};

#endif //DEBUG_H
