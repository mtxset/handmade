#include "tile.h"
#include "main.h"

global_var const i32 TILE_CHUNK_SAFE_MARGIN = INT32_MAX/64;
global_var const i32 TILE_CHUNK_UNINITIALIZED = INT32_MAX;

inline
Tile_chunk* get_tile_chunk(Tile_map* tile_map, i32 x, i32 y, i32 z, Memory_arena* arena = 0) {
    Tile_chunk* chunk = 0;
    
    u32 hash_value = 19 * x + 7 * y + 3 * z;
    u32 hash_slot = hash_value & (macro_array_count(tile_map->tile_chunk_hash) - 1);
    macro_assert(hash_slot < macro_array_count(tile_map->tile_chunk_hash));
    
    macro_assert(x > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z > -TILE_CHUNK_SAFE_MARGIN);
    macro_assert(x < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(y < TILE_CHUNK_SAFE_MARGIN);
    macro_assert(z < TILE_CHUNK_SAFE_MARGIN);
    
    chunk = tile_map->tile_chunk_hash + hash_slot;
    do {
        
        if (x == chunk->tile_chunk_x && 
            y == chunk->tile_chunk_y &&
            z == chunk->tile_chunk_z) {
            break;
        }
        
        if (arena && chunk->tile_chunk_x != TILE_CHUNK_UNINITIALIZED && !chunk->next_hash) {
            chunk->next_hash = push_struct(arena, Tile_chunk);
            chunk = chunk->next_hash;
            chunk->tile_chunk_x = TILE_CHUNK_UNINITIALIZED;
        }
        
        if (arena && chunk->tile_chunk_x == TILE_CHUNK_UNINITIALIZED) {
            u32 tile_count = tile_map->chunk_dimension * tile_map->chunk_dimension;
            
            chunk->tile_chunk_x = x;
            chunk->tile_chunk_y = y;
            chunk->tile_chunk_z = z;
            
            chunk->tiles = push_array(arena, tile_count, u32);
            for (u32 tile_index = 0; tile_index < tile_count; tile_index++) {
                chunk->tiles[tile_index] = 1;
            }
            
            chunk->next_hash = 0;
            
            break;
        }
        chunk = chunk->next_hash;
    } while (chunk);
    
    return chunk;
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

internal
Tile_chunk_position get_chunk_position_for(Tile_map* tile_map, u32 absolute_tile_x, u32 absolute_tile_y, u32 absolute_tile_z) {
    Tile_chunk_position result;
    
    result.tile_chunk_x = absolute_tile_x >> tile_map->chunk_shift;
    result.tile_chunk_y = absolute_tile_y >> tile_map->chunk_shift;
    result.tile_chunk_z = absolute_tile_z;
    
    result.tile_relative_x = absolute_tile_x & tile_map->chunk_mask;
    result.tile_relative_y = absolute_tile_y & tile_map->chunk_mask;
    
    return result;
}

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
void recanonicalize_coord(Tile_map* tile_map, i32* tile, f32* tile_relative) {
    i32 offset = round_f32_i32((*tile_relative) / tile_map->tile_side_meters);
    *tile += offset;
    *tile_relative -= offset * tile_map->tile_side_meters;
    
    // 0.5 cuz we wanna start from tile's center
    macro_assert(*tile_relative > -0.5f * tile_map->tile_side_meters);
    macro_assert(*tile_relative <  0.5f * tile_map->tile_side_meters);
}

inline
Tile_map_position map_into_tile_space(Tile_map* tile_map, Tile_map_position base_pos, v2 offset) {
    
    Tile_map_position result = base_pos;
    result._offset += offset;
    
    recanonicalize_coord(tile_map, &result.absolute_tile_x, &result._offset.x);
    recanonicalize_coord(tile_map, &result.absolute_tile_y, &result._offset.y);
    
    return result;
}

internal
void set_tile_value(Memory_arena* world_arena, Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y, u32 tile_abs_z, u32 tile_value) {
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y, tile_abs_z);
    
    Tile_chunk* tile_chunk = get_tile_chunk(tile_map, 
                                            chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z, 
                                            world_arena);
    
    set_tile_value(tile_map, tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y, tile_value);
}

internal
bool are_on_same_tile(Tile_map_position* pos_x, Tile_map_position* pos_y) {
    bool result = (pos_x->absolute_tile_x == pos_y->absolute_tile_x &&
                   pos_x->absolute_tile_y == pos_y->absolute_tile_y &&
                   pos_x->absolute_tile_z == pos_y->absolute_tile_z);
    return result;
}

internal
Tile_map_diff subtract_pos(Tile_map* tile_map, Tile_map_position* pos_a, Tile_map_position* pos_b) {
    Tile_map_diff result = {};
    
    v2 delta_tile_xy = { 
        (f32)pos_a->absolute_tile_x - (f32)pos_b->absolute_tile_x,
        (f32)pos_a->absolute_tile_y - (f32)pos_b->absolute_tile_y 
    };
    
    f32 delta_tile_z = (f32)pos_a->absolute_tile_z - (f32)pos_b->absolute_tile_z;
    
    result.xy = tile_map->tile_side_meters * delta_tile_xy + (pos_a->_offset - pos_b->_offset);
    
    result.z = tile_map->tile_side_meters * delta_tile_z;
    
    return result;
}

internal
Tile_map_position centered_tile_point(u32 absolute_tile_x, u32 absolute_tile_y, u32 absolute_tile_z) {
    
    Tile_map_position result = {};
    
    result.absolute_tile_x = absolute_tile_x;
    result.absolute_tile_y = absolute_tile_y;
    result.absolute_tile_z = absolute_tile_z;
    
    return result;
}

internal
void init_tile_map(Tile_map* tile_map, f32 tile_side_meters) {
    tile_map->chunk_shift = 4;
    tile_map->chunk_mask  = (1 << tile_map->chunk_shift) - 1;
    tile_map->chunk_dimension = (1 << tile_map->chunk_shift);
    tile_map->tile_side_meters = tile_side_meters;
    
    for (u32 tile_chunk_index = 0; tile_chunk_index < macro_array_count(tile_map->tile_chunk_hash); tile_chunk_index++) {
        tile_map->tile_chunk_hash[tile_chunk_index].tile_chunk_x = TILE_CHUNK_UNINITIALIZED;
    }
}