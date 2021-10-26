#include "world.h"
#include "main.h"

global_var const i32 TILE_CHUNK_SAFE_MARGIN = INT32_MAX/64;
global_var const i32 TILE_CHUNK_UNINITIALIZED = INT32_MAX;
global_var const i32 TILES_PER_CHUNK = 16;

inline
World_chunk* get_world_chunk(World* world, i32 x, i32 y, i32 z, Memory_arena* arena = 0) {
    macro_assert(x > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(x < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z < TILE_CHUNK_SAFE_MARGIN);
    
    u32 hash_value = 19 * x + 7 * y + 3 * z;
    u32 hash_slot = hash_value & (macro_array_count(world->chunk_hash) - 1);
    macro_assert(hash_slot < macro_array_count(world->chunk_hash));
    
    World_chunk* chunk = world->chunk_hash + hash_slot;
    do {
        
        if (x == chunk->chunk_x && 
            y == chunk->chunk_y &&
            z == chunk->chunk_z) {
            break;
        }
        
        if (arena && chunk->chunk_x != TILE_CHUNK_UNINITIALIZED && !chunk->next_hash) {
            chunk->next_hash = push_struct(arena, World_chunk);
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
bool is_canonical(World* world, f32 tile_relative) {
    bool result;
    // 0.5 cuz we wanna start from tile's center
    result = 
        tile_relative >= -0.5f * world->chunk_side_meters &&
        tile_relative <=  0.5f * world->chunk_side_meters;
    
    return result;
}


inline
bool is_canonical(World* world, v2 offset) {
    bool result;
    // 0.5 cuz we wanna start from tile's center
    result = is_canonical(world, offset.x) && is_canonical(world, offset.y);
    
    return result;
}

internal
void recanonicalize_coord(World* world, i32* tile, f32* tile_relative) {
    i32 offset = round_f32_i32((*tile_relative) / world->chunk_side_meters);
    *tile += offset;
    *tile_relative -= offset * world->chunk_side_meters;
    
    macro_assert(is_canonical(world, *tile_relative));
}

inline
World_position map_into_chunk_space(World* world, World_position base_pos, v2 offset) {
    
    World_position result = base_pos;
    result._offset += offset;
    
    recanonicalize_coord(world, &result.chunk_x, &result._offset.x);
    recanonicalize_coord(world, &result.chunk_y, &result._offset.y);
    
    return result;
}


internal
bool are_in_same_chunk(World* world, World_position* pos_x, World_position* pos_y) {
    macro_assert(is_canonical(world, pos_x->_offset));
    macro_assert(is_canonical(world, pos_y->_offset));
    
    bool result = (pos_x->chunk_x == pos_y->chunk_x &&
                   pos_x->chunk_y == pos_y->chunk_y &&
                   pos_x->chunk_z == pos_y->chunk_z);
    return result;
}

internal
World_position chunk_pos_from_tile_pos(World* world, i32 abs_tile_x, i32 abs_tile_y, i32 abs_tile_z) {
    World_position result = {};
    
    result.chunk_x = abs_tile_x / TILES_PER_CHUNK;
    result.chunk_y = abs_tile_y / TILES_PER_CHUNK;
    result.chunk_z = abs_tile_z / TILES_PER_CHUNK;
    
    result._offset.x = (f32)(abs_tile_x - (result.chunk_x * TILES_PER_CHUNK)) * world->tile_side_meters;
    result._offset.y = (f32)(abs_tile_y - (result.chunk_y * TILES_PER_CHUNK)) * world->tile_side_meters;
    
    return result;
}

internal
World_diff subtract_pos(World* world, World_position* pos_a, World_position* pos_b) {
    World_diff result = {};
    
    v2 delta_tile_xy = { 
        (f32)pos_a->chunk_x - (f32)pos_b->chunk_x,
        (f32)pos_a->chunk_y - (f32)pos_b->chunk_y 
    };
    
    f32 delta_tile_z = (f32)pos_a->chunk_z - (f32)pos_b->chunk_z;
    
    result.xy = world->chunk_side_meters * delta_tile_xy + (pos_a->_offset - pos_b->_offset);
    
    result.z = world->chunk_side_meters * delta_tile_z;
    
    return result;
}

internal
World_position centered_chunk_point(u32 chunk_x, u32 chunk_y, u32 chunk_z) {
    
    World_position result = {};
    
    result.chunk_x = chunk_x;
    result.chunk_y = chunk_y;
    result.chunk_z = chunk_z;
    
    return result;
}

internal
void init_world(World* world, f32 tile_side_meters) {
    world->tile_side_meters = tile_side_meters;
    world->chunk_side_meters = (f32)TILES_PER_CHUNK * tile_side_meters;
    world->first_free = 0;
    
    for (u32 tile_chunk_index = 0; tile_chunk_index < macro_array_count(world->chunk_hash); tile_chunk_index++) {
        world->chunk_hash[tile_chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
        world->chunk_hash[tile_chunk_index].first_block.entity_count = 0;
    }
}

inline
void change_entity_location(Memory_arena* arena, World* world, u32 low_entity_index, World_position* old_pos, World_position* new_pos) {
    if (old_pos && are_in_same_chunk(world, old_pos, new_pos)) {
        // no need to do anything
    }
    else {
        if (old_pos) {
            World_chunk* chunk = get_world_chunk(world, old_pos->chunk_x, old_pos->chunk_y, old_pos->chunk_z);
            macro_assert(chunk);
            
            if (chunk) {
                bool not_found = true;
                World_entity_block* first_block = &chunk->first_block;
                
                for (World_entity_block* block = first_block; block && not_found; block = block->next) {
                    for (u32 index = 0; index < block->entity_count && not_found; index++) {
                        if (block->low_entity_index[index] == low_entity_index) {
                            macro_assert(first_block->entity_count > 0);
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
        
        World_chunk* chunk = get_world_chunk(world, new_pos->chunk_x, new_pos->chunk_y, new_pos->chunk_z, arena);
        macro_assert(chunk);
        
        World_entity_block* block = &chunk->first_block;
        if (block->entity_count == macro_array_count(block->low_entity_index)) {
            // new block
            World_entity_block* old_block = world->first_free;
            
            if (old_block) {
                world->first_free = old_block->next;
            }
            else {
                old_block = push_struct(arena, World_entity_block);
            }
            
            *old_block = *block;
            block->next = old_block;
            block->entity_count = 0;
        }
        
        macro_assert(block->entity_count < macro_array_count(block->low_entity_index));
        block->low_entity_index[block->entity_count++] = low_entity_index;
    }
}