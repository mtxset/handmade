#include "tile.h"

Tile_chunk* get_tile_chunk(Tile_map* tile_map, u32 x, u32 y) {
    Tile_chunk* result = 0;
    
    if (x >= 0 && x < tile_map->tile_chunk_count_x &&
        y >= 0 && y < tile_map->tile_chunk_count_y) {
        result = &tile_map->tile_chunks[y * tile_map->tile_chunk_count_x + x];
    }
    
    return result;
}

u32 get_tile_entry_unchecked(Tile_map* world, Tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y) {
    macro_assert(chunk);
    macro_assert(player_tile_x < world->chunk_dimension);
    macro_assert(player_tile_y < world->chunk_dimension);
    
    u32 result;
    
    result = chunk->tiles[player_tile_y * world->chunk_dimension + player_tile_x];
    
    return result;
}

void set_tile_entry_unchecked(Tile_map* world, Tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y, u32 tile_value) {
    macro_assert(chunk);
    macro_assert(player_tile_x < world->chunk_dimension);
    macro_assert(player_tile_y < world->chunk_dimension);
    
    chunk->tiles[player_tile_y * world->chunk_dimension + player_tile_x] = tile_value;
}

Tile_chunk_position get_chunk_position_for(Tile_map* world_data, u32 absolute_tile_x, u32 absolute_tile_y) {
    Tile_chunk_position result;
    
    result.tile_chunk_x = absolute_tile_x >> world_data->chunk_shift;
    result.tile_chunk_y = absolute_tile_y >> world_data->chunk_shift;
    
    result.tile_relative_x = absolute_tile_x & world_data->chunk_mask;
    result.tile_relative_y = absolute_tile_y & world_data->chunk_mask;
    
    return result;
}

static
bool get_tile_value(Tile_map* world, Tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y) {
    u32 result = 0;
    
    if (!chunk)
        return 0;
    
    result = get_tile_entry_unchecked(world, chunk, player_tile_x, player_tile_y);
    
    return result;
}

static
void set_tile_value(Tile_map* world, Tile_chunk* chunk, u32 player_tile_x, u32 player_tile_y, u32 tile_value) {
    u32 result = 0;
    
    if (!chunk)
        return;
    
    set_tile_entry_unchecked(world, chunk, player_tile_x, player_tile_y, tile_value);
}

static
u32 get_tile_value(Tile_map* tile_map, u32 tile_abs_x, u32 tile_abs_y) {
    bool result = false;
    
    Tile_chunk_position chunk_pos = get_chunk_position_for(tile_map, tile_abs_x, tile_abs_y);
    Tile_chunk* test_tile_chunk = get_tile_chunk(tile_map, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    result = get_tile_value(tile_map, test_tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y);
    
    return result;
}

static
bool is_world_point_empty(Tile_map* tile_map, Tile_map_position can_pos) {
    bool result = false;
    
    u32 tile_chunk_value = get_tile_value(tile_map, can_pos.absolute_tile_x, can_pos.absolute_tile_y);
    
    result = (tile_chunk_value == 0);
    
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
    
    set_tile_value(tile_map, test_tile_chunk, chunk_pos.tile_relative_x, chunk_pos.tile_relative_y, tile_value);
}