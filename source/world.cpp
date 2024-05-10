
#include "stdint.h"

global_var const i32 TILE_CHUNK_SAFE_MARGIN = (INT32_MAX / 64);
global_var const i32 TILE_CHUNK_UNINITIALIZED = INT32_MAX;
global_var const i32 TILES_PER_CHUNK = 8;

inline
bool is_position_valid(World_position pos) {
  bool result = pos.chunk_x != TILE_CHUNK_UNINITIALIZED;
  
  return result;
}

inline
World_position null_position() {
  World_position result = {};
  
  result.chunk_x = TILE_CHUNK_UNINITIALIZED;
  
  return result;
};

inline
World_chunk* get_world_chunk(World* world, i32 x, i32 y, i32 z, Memory_arena* arena = 0) {
  assert(x > -TILE_CHUNK_SAFE_MARGIN);
  assert(y > -TILE_CHUNK_SAFE_MARGIN);
  assert(z > -TILE_CHUNK_SAFE_MARGIN);
  assert(x < TILE_CHUNK_SAFE_MARGIN);
  assert(y < TILE_CHUNK_SAFE_MARGIN);
  assert(z < TILE_CHUNK_SAFE_MARGIN);
  
  u32 hash_value = 19 * x + 7 * y + 3 * z;
  u32 hash_slot = hash_value & (array_count(world->chunk_hash) - 1);
  assert(hash_slot < array_count(world->chunk_hash));
  
  World_chunk* chunk = world->chunk_hash + hash_slot;
  do {
    
    if (x == chunk->chunk_x && 
        y == chunk->chunk_y &&
        z == chunk->chunk_z) {
      break;
    }
    
    if (arena && chunk->chunk_x != TILE_CHUNK_UNINITIALIZED && !chunk->next_hash) {
      chunk->next_hash = mem_push_struct(arena, World_chunk);
      chunk = chunk->next_hash;
      chunk->chunk_x = TILE_CHUNK_UNINITIALIZED;
    }
    
    if (arena && chunk->chunk_x == TILE_CHUNK_UNINITIALIZED) {
      chunk->chunk_x = x;
      chunk->chunk_y = y;
      chunk->chunk_z = z;
      
      chunk->next_hash = 0;
      break;
    }
    chunk = chunk->next_hash;
  } while (chunk);
  
  return chunk;
}

inline
bool is_canonical(f32 chunk_dim, f32 tile_relative) {
  bool result;
  f32 epsilon = 0.01f;
  // 0.5 cuz we wanna start from tile's center
  result = 
    tile_relative >= -(0.5f * chunk_dim + epsilon) &&
    tile_relative <=  (0.5f * chunk_dim + epsilon);
  
  return result;
}

inline
bool is_canonical(World* world, v3 offset) {
  bool result;
  // 0.5 cuz we wanna start from tile's center
  result = 
    is_canonical(world->chunk_dim_meters.x, offset.x) && 
    is_canonical(world->chunk_dim_meters.y, offset.y) &&
    is_canonical(world->chunk_dim_meters.z, offset.z);
  
  return result;
}

internal
void 
recanonicalize_coord(f32 chunk_dim, i32* tile, f32* tile_relative) {
  i32 offset = round_f32_i32((*tile_relative) / chunk_dim);
  *tile += offset;
  *tile_relative -= offset * chunk_dim;
  
  assert(is_canonical(chunk_dim, *tile_relative));
}

inline
World_position 
map_into_chunk_space(World* world, World_position base_pos, v3 offset) {
  
  World_position result = base_pos;
  result._offset += offset;
  
  recanonicalize_coord(world->chunk_dim_meters.x, &result.chunk_x, &result._offset.x);
  recanonicalize_coord(world->chunk_dim_meters.y, &result.chunk_y, &result._offset.y);
  recanonicalize_coord(world->chunk_dim_meters.z, &result.chunk_z, &result._offset.z);
  
  return result;
}

internal
bool are_in_same_chunk(World* world, World_position* pos_x, World_position* pos_y) {
  
  assert(is_canonical(world, pos_x->_offset));
  assert(is_canonical(world, pos_y->_offset));
  
  bool result = (pos_x->chunk_x == pos_y->chunk_x &&
                 pos_x->chunk_y == pos_y->chunk_y &&
                 pos_x->chunk_z == pos_y->chunk_z);
  return result;
}

internal
v3
subtract_pos(World* world, World_position* pos_a, World_position* pos_b) {
  v3 result = {};
  
  v3 delta_tile = { 
    (f32)pos_a->chunk_x - (f32)pos_b->chunk_x,
    (f32)pos_a->chunk_y - (f32)pos_b->chunk_y,
    (f32)pos_a->chunk_z - (f32)pos_b->chunk_z
  };
  
  result = hadamard(world->chunk_dim_meters, delta_tile) + (pos_a->_offset - pos_b->_offset);
  
  return result;
}

internal
World_position 
centered_chunk_point(u32 chunk_x, u32 chunk_y, u32 chunk_z) {
  
  World_position result = {};
  
  result.chunk_x = chunk_x;
  result.chunk_y = chunk_y;
  result.chunk_z = chunk_z;
  
  return result;
}

internal
World_position 
centered_chunk_point(World_chunk* chunk) {
  
  World_position result = {};
  
  result = centered_chunk_point(chunk->chunk_x, chunk->chunk_y, chunk->chunk_z);
  
  return result;
}

internal
void init_world(World* world, v3 chunk_dim_meters) {
  world->chunk_dim_meters = chunk_dim_meters;
  world->first_free = 0;
  
  for (u32 tile_chunk_index = 0; tile_chunk_index < array_count(world->chunk_hash); tile_chunk_index++) {
    world->chunk_hash[tile_chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
    world->chunk_hash[tile_chunk_index].first_block.entity_count = 0;
  }
}

inline
void change_entity_location_raw(Memory_arena* arena, World* world, u32 low_entity_index, World_position* old_pos, World_position* new_pos) {
  
  assert(!old_pos || is_position_valid(*old_pos));
  assert(!new_pos || is_position_valid(*new_pos));
  
  if (old_pos && new_pos && are_in_same_chunk(world, old_pos, new_pos)) {
    // no need to do anything
  }
  else {
    if (old_pos) {
      World_chunk* chunk = get_world_chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
      assert(chunk);
      
      if (chunk) {
        bool not_found = true;
        World_entity_block* first_block = &chunk->first_block;
        
        for (World_entity_block* block = first_block; block && not_found; block = block->next) {
          for (u32 index = 0; index < block->entity_count && not_found; index++) {
            
            if (block->low_entity_index[index] == low_entity_index) {
              
              assert(first_block->entity_count > 0);
              block->low_entity_index[index] = first_block->low_entity_index[--first_block->entity_count];
              
              if (first_block->entity_count == 0) {
                if (first_block->next) {
                  World_entity_block* next_block = first_block->next;
                  *first_block = *next_block;
                  
                  next_block->next = world->first_free;
                  world->first_free = next_block;
                }
              }
              
              not_found = false;
            }
          }
        }
      }
    }
    
    if (new_pos) {
      World_chunk* chunk = get_world_chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
      assert(chunk);
      
      World_entity_block* block = &chunk->first_block;
      if (block->entity_count == array_count(block->low_entity_index)) {
        // new block
        World_entity_block* old_block = world->first_free;
        
        if (old_block) {
          world->first_free = old_block->next;
        }
        else {
          old_block = mem_push_struct(arena, World_entity_block);
        }
        
        *old_block = *block;
        block->next = old_block;
        block->entity_count = 0;
      }
      
      assert(block->entity_count < array_count(block->low_entity_index));
      block->low_entity_index[block->entity_count++] = low_entity_index;
    }
  }
}

internal
void 
change_entity_location(Memory_arena* arena, World* world, u32 low_entity_index, Low_entity* low_entity, World_position new_pos_init) {
  
  World_position* old_pos = 0;
  World_position* new_pos = 0;
  
  if (!is_set(&low_entity->sim, Entity_flag_non_spatial) && 
      is_position_valid(low_entity->position)) {
    old_pos = &low_entity->position;
  }
  
  if (is_position_valid(new_pos_init)) {
    new_pos = &new_pos_init;
  }
  
  change_entity_location_raw(arena, world, low_entity_index, old_pos, new_pos);
  
  if (new_pos) {
    low_entity->position = *new_pos;
    clear_flags(&low_entity->sim, Entity_flag_non_spatial);
  }
  else {
    low_entity->position = null_position();
    add_flags(&low_entity->sim, Entity_flag_non_spatial);
  }
}