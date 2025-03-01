/* date = March 1st 2025 11:42 pm */

#ifndef DEBUG_H
#define DEBUG_H


#if (!COMPILER_MSVC && !COMPILER_LLVM)
assert(!"not supported")
#endif

#if INTERNAL
#define timed_block(id) Timed_block Timed_block##id(Debug_cycle_counter_type_##id);
#define stop_timed_block(id) Timed_block##id.end()
#else
#define timed_block(id)
#endif

struct Timed_block {
  u64 start_cycle_count;
  u32 id;
  bool ended = false;
  
  Timed_block(u32 param_id) {
    id = param_id;
    start_cycle_count = __rdtsc();
  }
  
  ~Timed_block() {
    if (ended)
      return;
    
    debug_global_memory->counter_list[id].cycle_count += __rdtsc() - start_cycle_count;
    debug_global_memory->counter_list[id].hit_count++;
  }
  
  void end() {
    ended = true;
    
    debug_global_memory->counter_list[id].cycle_count += __rdtsc() - start_cycle_count;
    debug_global_memory->counter_list[id].hit_count++;
  }
  
};

#endif //DEBUG_H
