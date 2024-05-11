/* date = February 12th 2024 1:57 pm */

#ifndef MEMORY_H
#define MEMORY_H

struct Memory_arena {
  u8* base;
  size_t size;
  size_t used;
  
  u32 temp_count;
};

struct Temp_memory {
  Memory_arena* arena;
  size_t used;
};


inline
void initialize_arena(Memory_arena* arena, size_t size, void* base) {
  arena->size = size;
  arena->base = (u8*)base;
  arena->used = 0;
  arena->temp_count = 0;
}

#define mem_push_struct(arena,type,...) \
(type *)mem_push_size_(arena, sizeof(type), ## __VA_ARGS__)

#define mem_push_array(arena,count,type, ...) \
(type *)mem_push_size_(arena, (count) * sizeof(type), ## __VA_ARGS__)

#define mem_push_size(arena,size, ...) \
mem_push_size_(arena, size, ## __VA_ARGS__)

#define mem_zero_struct(instance) mem_zero_size_(sizeof(instance), &(instance))

inline
size_t
get_alignment_offset(Memory_arena *arena, size_t alignment = 4) {
  size_t result_pointer = (size_t)arena->base + arena->used;
  size_t current_alignment = result_pointer & (alignment - 1);
  size_t alignment_offset = (alignment - current_alignment) & (alignment - 1);
  
  return alignment_offset;
}

inline
void* 
mem_push_size_(Memory_arena* arena, size_t size_init, size_t alignment = 4) {
  
  size_t size = size_init;
  
  size_t alignment_offset = get_alignment_offset(arena, alignment);
  size += alignment_offset;
  
  assert((arena->used + size) <= arena->size);
  
  void* result = arena->base + arena->used + alignment_offset;
  arena->used += size;
  
  assert(size >= size_init);
  
  return result;
}

inline 
char*
mem_push_string(Memory_arena *arena, char *src)
{
  u32 size = 1;
  for (char *at = src; *at; ++at) {
    ++size;
  }
  
  char *dst = (char*)mem_push_size_(arena, size);
  for (u32 char_index = 0; char_index < size; ++char_index) {
    dst[char_index] = src[char_index];
  }
  
  return dst;
}

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
  assert(arena->used >= temp_memory.used);
  arena->used = temp_memory.used;
  assert(arena->temp_count > 0);
  arena->temp_count--;
}

inline
void
check_arena(Memory_arena* arena) {
  assert(arena->temp_count == 0);
}

inline
size_t
get_arena_size_remaining(Memory_arena *arena, size_t alignment = 4) {
  size_t result = 0;
  
  size_t alignment_offset = get_alignment_offset(arena, alignment);
  result = arena->size - (arena->used + alignment_offset);
  
  return result;
}


inline
void
sub_arena(Memory_arena* result, Memory_arena *arena, size_t size, size_t alignment = 16) {
  result->size = size;
  result->base = (u8*)mem_push_size(arena, size, alignment);
  result->used = 0;
  result->temp_count = 0;
}

inline
void 
mem_zero_size_(size_t size, void* pointer) {
  u8* byte = (u8*)pointer;
  while (size--)
    *byte++ = 0;
}


#endif //MEMORY_H
