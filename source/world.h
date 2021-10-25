/* date = September 15th 2021 6:06 pm */

#ifndef TILE_H
#define TILE_H

#include "vectors.h"

struct World_diff {
    v2 xy;
    f32 z;
};

struct World_position {
    i32 chunk_x;
    i32 chunk_y;
    i32 chunk_z;
    
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
    f32 tile_side_meters;
    f32 chunk_side_meters;
    
    World_entity_block* first_free;
    
    World_chunk chunk_hash[4096];
};

#endif //TILE_H
