/* date = September 15th 2021 6:06 pm */

#ifndef TILE_H
#define TILE_H

struct Tile_map_position {
    u32 absolute_tile_x;
    u32 absolute_tile_y;
    
    f32 tile_relative_x; 
    f32 tile_relative_y;
};

struct Tile_chunk {
    u32* tiles;
};

struct Tile_chunk_position {
    // 24 high bits are tile chunk, 8 low bits are tile index of that chunk
    u32 tile_chunk_x;
    u32 tile_chunk_y;
    
    u32 tile_relative_x;
    u32 tile_relative_y;
};

struct Tile_map {
    u32 chunk_shift;     // world chunk
    u32 chunk_mask;      // tiles
    u32 chunk_dimension; // how many tiles in a world chunk
    
    f32 tile_radius_meters;
    f32 tile_side_meters;
    i32 tile_side_pixels;
    f32 meters_to_pixels;
    
    u32 tile_chunk_count_x;
    u32 tile_chunk_count_y;
    
    Tile_chunk* tile_chunks;
};

#endif //TILE_H
