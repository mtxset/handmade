/* date = September 15th 2021 6:06 pm */

#ifndef TILE_H
#define TILE_H

#include "vectors.h"

struct World_diff {
    v2 xy;
    f32 z;
};

struct World_position {
    i32 absolute_tile_x;
    i32 absolute_tile_y;
    i32 absolute_tile_z;
    
    v2 _offset; 
};

struct World_entity_block {
    u32 entity_count;
    u32 low_entity_index[16];
    World_entity_block* next;
};

struct World_chunk {
    i32 chunk_x;
    i32 chunk_y;
    i32 chunk_z;
    
    World_entity_block first_block;
    World_chunk* next_hash;
};

struct World {
    i32 chunk_shift;     // world chunk
    i32 chunk_mask;      // tiles
    i32 chunk_dimension; // how many tiles in a world chunk
    
    f32 tile_side_meters;
    
    World_chunk chunk_hash[4096];
};

#endif //TILE_H
