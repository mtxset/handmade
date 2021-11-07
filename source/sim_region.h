/* date = November 5th 2021 5:04 pm */

#ifndef SIM_REGION_H
#define SIM_REGION_H

#include "types.h"

struct Sim_entity {
    u32 storage_index;
    
    v2 position;
    v2 velocity_d;
    u32 chunk_z;
    
    f32 z;
    f32 z_velocity_d;
};

struct Sim_region {
    World* world;
    World_position origin;
    Rect2 bounds;
    
    u32 entity_count;
    u32 max_entity_count;
    
    Sim_entity* entity_list;
};

#endif //SIM_REGION_H
