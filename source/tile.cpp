#include "tile.h"
#include "main.h"

Tile_chunk* get_tile_chunk(Tile_map* tile_map, u32 x, u32 y) {
    Tile_chunk* result = 0;
    
    if (x < tile_map->tile_chunk_count_x &&
        y < tile_map->tile_chunk_count_y) {
        result = &tile_map->tile_chunks[y * tile_map->tile_chunk_count_x + x];
    }
    
    return result;
}

u32 get_tile_entry_unchecked(Tile_map* tile_map, Tile_chunk* chunk, u32 tile_x, u32 tile_y) {
    macro_assert(chunk);
    macro_assert(tile_x < tile_map->chunk_dimension);
    macro_assert(tile_y < tile_map->chunk_dimension);
    
    u32 result;
    
    result = chunk->tiles[tile_y * tile_map->chunk_dimension + tile_x];
    
    return result;
}

void set_tile_entry_unchecked(Tile_map* tile_map, Tile_chunk* chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    macro_assert(chunk);
    macro_assert(tile_x < tile_map->chunk_dimension);
    macro_assert(tile_y < tile_map->chunk_dimension);
    
    chunk->tiles[tile_y * tile_map->chunk_dimension + tile_x] = tile_value;
    auto set_value = get_tile_entry_unchecked(tile_map, chunk, tile_x, tile_y);
    macro_assert(tile_value == set_value);
}

Tile_chunk_position get_chunk_position_for(Tile_map* tile_map, u32 absolute_tile_x, u32 absolute_tile_y) {
    Tile_chunk_position result;
    
    result.tile_chunk_x = absolute_tile_x >> tile_map->chunk_shift;
    result.tile_chunk_y = absolute_tile_y >> tile_map->chunk_shift;
    
    result.tile_relative_x = absolute_tile_x & tile_map->chunk_mask;
    result.tile_relative_y = absolute_tile_y & tile_map->chunk_mask;
    
    return result;
}

static
u32 get_tile_value(Tile_map* tile_map, Tile_chunk* tile_chunk, u32 tile_x, u32 tile_y) {
    u32 result = 0;
    
    if (!tile_chunk || !tile_chunk->tiles)
        return 0;
    
    result = get_tile_entry_unchecked(tile_map, tile_chunk, tile_x, tile_y);
    
    return result;
}

static
u32 get_tile_value(Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y) {
    u32 result = 0;
    
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y);
    Tile_chunk* test_tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    result = get_tile_value(tile_map, test_tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y);
    
    return result;
}

static
void set_tile_value(Tile_map* tile_map, Tile_chunk* tile_chunk, u32 tile_x, u32 tile_y, u32 tile_value) {
    if (!tile_chunk || !tile_chunk->tiles)
        return;
    
    set_tile_entry_unchecked(tile_map, tile_chunk, tile_x, tile_y, tile_value);
}

static
bool is_world_point_empty(Tile_map* tile_map, Tile_map_position can_pos) {
    bool result = false;
    
    u32 tile_chunk_value = get_tile_value(tile_map, can_pos.absolute_tile_x, can_pos.absolute_tile_y);
    
    result = (tile_chunk_value == 1);
    
    return result;
}

void recanonicalize_coord(Tile_map* tile_map, u32* tile, f32* tile_relative) {
    i32 offset = round_f32_i32((*tile_relative) / tile_map->tile_side_meters);
    *tile += offset;
    *tile_relative -= offset * tile_map->tile_side_meters;
    
    // 0.5 cuz we wanna start from tile's center
    macro_assert(*tile_relative >= -0.5f * tile_map->tile_side_meters);
    macro_assert(*tile_relative <=  0.5f * tile_map->tile_side_meters);
}

Tile_map_position recanonicalize_position(Tile_map* tile_map, Tile_map_position pos) {
    
    Tile_map_position result = pos;
    
    recanonicalize_coord(tile_map, &result.absolute_tile_x, &result.tile_relative_x);
    recanonicalize_coord(tile_map, &result.absolute_tile_y, &result.tile_relative_y);
    
    return result;
}

static
void set_tile_value(Memory_arena* world_arena, Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y, u32 tile_value) {
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y);
    Tile_chunk* test_tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    
    macro_assert(test_tile_chunk);
    
    if (!test_tile_chunk->tiles) {
        u32 array_size = tile_map->chunk_dimension * tile_map->chunk_dimension;
        test_tile_chunk->tiles = push_array(world_arena, array_size, u32);
        
        for (u32 i = 0; i < array_size; i++) {
            test_tile_chunk->tiles[i] = 1;
        }
    }
    
    set_tile_value(tile_map, test_tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y, tile_value);
}