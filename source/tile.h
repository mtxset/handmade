/* date = September 15th 2021 6:06 pm */

#ifndef TILE_H
#define TILE_H

#include "vectors.h"

struct Tile_map_diff {
    v2 xy;
    f32 z;
};

struct Tile_map_position {
    i32 absolute_tile_x;
    i32 absolute_tile_y;
    i32 absolute_tile_z;
    
    v2 _offset; 
};

struct Tile_chunk {
    i32 tile_chunk_x;
    i32 tile_chunk_y;
    i32 tile_chunk_z;
    
    u32* tiles;
    
    Tile_chunk* next_hash;
};

struct Tile_chunk_position {
    // 24 high bits are tile chunk, 8 low bits are tile index of that chunk
    i32 tile_chunk_x;
    i32 tile_chunk_y;
    i32 tile_chunk_z;
    
    i32 tile_relative_x;
    i32 tile_relative_y;
};

struct Tile_map {
    i32 chunk_shift;     // world chunk
    i32 chunk_mask;      // tiles
    i32 chunk_dimension; // how many tiles in a world chunk
    
    f32 tile_side_meters;
    
    Tile_chunk tile_chunk_hash[4096];
};

#endif //TILE_H
