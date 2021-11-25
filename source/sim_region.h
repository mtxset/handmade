/* date = November 5th 2021 5:04 pm */

#ifndef SIM_REGION_H
#define SIM_REGION_H

#include "intrinsics.h"

struct Move_spec {
    bool max_acceleration_vector;
    f32 speed;
    f32 drag;
    f32 boost;
};

enum Entity_type {
    Entity_type_null,
    Entity_type_hero,
    Entity_type_wall,
    Entity_type_familiar,
    Entity_type_monster,
    Entity_type_sword
};

#define HIT_POINT_SUB_COUNT 4
struct Hit_point {
    u8 flags;
    u8 filled_amount;
};

struct Sim_entity;
union Entity_ref {
    Sim_entity* pointer;
    u32 index;
};

enum Sim_entity_flags {
    Entity_flag_collides    = (1 << 1),
    Entity_flag_non_spatial = (1 << 2),
    
    Entity_flag_simming     = (1 << 30),
};

struct Sim_entity {
    u32 storage_index;
    bool updatable;
    
    v2 position;
    v2 velocity_d;
    u32 chunk_z;
    
    f32 z;
    f32 z_velocity_d;
    
    f32 distance_limit;
    
    Entity_type type;
    u32 flags;
    
    f32 width, height;
    
    i32 d_abs_tile_z;
    
    u32 hit_points_max;
    Hit_point hit_point[16];
    
    u32 facing_direction;
    f32 t_bob;
    
    Entity_ref sword;
};

struct Sim_entity_hash {
    Sim_entity* pointer;
    u32 index;
};

struct Sim_region {
    World* world;
    World_position origin;
    
    Rect2 bounds;
    Rect2 updatable_bounds;
    
    u32 entity_count;
    u32 max_entity_count;
    
    Sim_entity* entity_list;
    
    Sim_entity_hash hash[4000];
};

#endif //SIM_REGION_H
