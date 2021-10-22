#include "world.h"
#include "main.h"

global_var const i32 TILE_CHUNK_SAFE_MARGIN = INT32_MAX/64;
global_var const i32 TILE_CHUNK_UNINITIALIZED = INT32_MAX;

inline
World_chunk* get_tile_chunk(World* world, i32 x, i32 y, i32 z, Memory_arena* arena = 0) {
    World_chunk* chunk = 0;
    
    u32 hash_value = 19 * x + 7 * y + 3 * z;
    u32 hash_slot = hash_value & (macro_array_count(world->chunk_hash) - 1);
    macro_assert(hash_slot < macro_array_count(world->chunk_hash));
    
    macro_assert(x > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(x < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z < TILE_CHUNK_SAFE_MARGIN);
    
    chunk = world->chunk_hash + hash_slot;
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
            u32 tile_count = world->chunk_dimension * world->chunk_dimension;
            
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

#if 0
internal
World_position get_chunk_position_for(World* world, u32 absolute_tile_x, u32 absolute_tile_y, u32 absolute_tile_z) {
    World_position result;
    
    result.chunk_x = absolute_tile_x >> world->chunk_shift;
    result.chunk_y = absolute_tile_y >> world->chunk_shift;
    result.chunk_z = absolute_tile_z;
    
    result.tile_relative_x = absolute_tile_x & world->chunk_mask;
    result.tile_relative_y = absolute_tile_y & world->chunk_mask;
    
    return result;
}
#endif

internal
void recanonicalize_coord(World* world, i32* tile, f32* tile_relative) {
    i32 offset = round_f32_i32((*tile_relative) / world->tile_side_meters);
    *tile += offset;
    *tile_relative -= offset * world->tile_side_meters;
    
    // 0.5 cuz we wanna start from tile's center
    macro_assert(*tile_relative > -0.5f * world->tile_side_meters);
    macro_assert(*tile_relative <  0.5f * world->tile_side_meters);
}

inline
World_position map_into_tile_space(World* world, World_position base_pos, v2 offset) {
    
    World_position result = base_pos;
    result._offset += offset;
    
    recanonicalize_coord(world, &result.absolute_tile_x, &result._offset.x);
    recanonicalize_coord(world, &result.absolute_tile_y, &result._offset.y);
    
    return result;
}


internal
bool are_on_same_tile(World_position* pos_x, World_position* pos_y) {
    bool result = (pos_x->absolute_tile_x == pos_y->absolute_tile_x &&
                   pos_x->absolute_tile_y == pos_y->absolute_tile_y &&
                   pos_x->absolute_tile_z == pos_y->absolute_tile_z);
    return result;
}

internal
World_diff subtract_pos(World* world, World_position* pos_a, World_position* pos_b) {
    World_diff result = {};
    
    v2 delta_tile_xy = { 
        (f32)pos_a->absolute_tile_x - (f32)pos_b->absolute_tile_x,
        (f32)pos_a->absolute_tile_y - (f32)pos_b->absolute_tile_y 
    };
    
    f32 delta_tile_z = (f32)pos_a->absolute_tile_z - (f32)pos_b->absolute_tile_z;
    
    result.xy = world->tile_side_meters * delta_tile_xy + (pos_a->_offset - pos_b->_offset);
    
    result.z = world->tile_side_meters * delta_tile_z;
    
    return result;
}

internal
World_position centered_tile_point(u32 absolute_tile_x, u32 absolute_tile_y, u32 absolute_tile_z) {
    
    World_position result = {};
    
    result.absolute_tile_x = absolute_tile_x;
    result.absolute_tile_y = absolute_tile_y;
    result.absolute_tile_z = absolute_tile_z;
    
    return result;
}

internal
void init_world(World* world, f32 tile_side_meters) {
    world->chunk_shift = 4;
    world->chunk_mask  = (1 << world->chunk_shift) - 1;
    world->chunk_dimension = (1 << world->chunk_shift);
    world->tile_side_meters = tile_side_meters;
    
    for (u32 tile_chunk_index = 0; tile_chunk_index < macro_array_count(world->chunk_hash); tile_chunk_index++) {
        world->chunk_hash[tile_chunk_index].chunk_x = TILE_CHUNK_UNINITIALIZED;
    }
}

// @cleanup
/*
internal
void set_tile_value(Memory_arena* world_arena, Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y, u32 tile_abs_z, u32 tile_value) {
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y, tile_abs_z);
    
    Tile_chunk* tile_chunk = get_tile_chunk(tile_map, 
                                            chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z, 
                                            world_arena);
    
    set_tile_value(tile_map, tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y, tile_value);
}
#endif 

inline
u32 get_tile_value(Tile_map* tile_map, Tile_chunk* tile_chunk, u32 tile_x, u32 tile_y) {
    u32 result = 0;
    
    if (!tile_chunk || !tile_chunk->tiles)
        return 0;
    
    result = get_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y);
    
    return result;
}

inline
u32 get_tile_value(Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y, u32 tile_abs_z) {
    u32 result = 0;
    
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y, tile_abs_z);
    Tile_chunk* tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
    
    result = get_tile_value(tile_map, tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y);
    
    return result;
}

inline
u32 get_tile_value(Tile_map* tile_map, Tile_map_position tile_pos) {
    
    u32 result = get_tile_value(tile_map, tile_pos.absolute_tile_x, tile_pos.absolute_tile_y, tile_pos.absolute_tile_z);
    
    return result;
}


internal
void set_tile_value(Tile_map* tile_map, Tile_chunk* tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    if (!tile_chunk || !tile_chunk->tiles)
        return;
    
    set_tile_value_unchecked(tile_map, tile_chunk, tile_x, tile_y, tile_value);
}

internal
bool is_tile_map_empty(u32 tile_value) {
    bool result = false;
    
    result = (tile_value == 1 || tile_value == 3 || tile_value == 4);
    
    return result;
}

internal
bool is_world_point_empty(Tile_map* tile_map, Tile_map_position tile_pos) {
    bool result = false;
    
    u32 tile_chunk_value = get_tile_value(tile_map, tile_pos);
    
    result = is_tile_map_empty(tile_chunk_value);
    
    return result;
}


internal
u32 get_tile_value_unchecked(Tile_map* tile_map, Tile_chunk* chunk, i32 tile_x, i32 tile_y) {
    macro_assert(chunk);
    macro_assert(tile_x < tile_map->chunk_dimension);
    macro_assert(tile_y < tile_map->chunk_dimension);
    
    u32 result;
    
    u32 index = tile_y * tile_map->chunk_dimension + tile_x;
    
    result = chunk->tiles[index];
    
    return result;
}

internal
void set_tile_value_unchecked(Tile_map* tile_map, Tile_chunk* chunk, i32 tile_x, i32 tile_y, u32 tile_value) {
    macro_assert(chunk);
    macro_assert(tile_x < tile_map->chunk_dimension);
    macro_assert(tile_y < tile_map->chunk_dimension);
    
    u32 index = tile_y * tile_map->chunk_dimension + tile_x;
    chunk->tiles[index] = tile_value;
    macro_assert(tile_value == get_tile_value_unchecked(tile_map, chunk, tile_x, tile_y));
}
 

*/