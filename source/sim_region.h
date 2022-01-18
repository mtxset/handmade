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
    
    Entity_type_space,
    Entity_type_hero,
    Entity_type_wall,
    Entity_type_familiar,
    Entity_type_monster,
    Entity_type_sword,
    Entity_type_stairwell,
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
    Entity_flag_collides    = (1 << 0),
    Entity_flag_non_spatial = (1 << 1),
    Entity_flag_moveable    = (1 << 2),
    Entity_flag_zsupported  = (1 << 3),
    Entity_flag_traversable = (1 << 4),
    
    Entity_flag_simming     = (1 << 30),
};

struct Sim_entity_collision_volume {
    v3 offset_pos;
    v3 dim;
};

struct Sim_entity_collision_group {
    
    Sim_entity_collision_volume total_volume;
    
    u32 volume_count;
    Sim_entity_collision_volume* volume_list;
};

struct Sim_entity {
    u32 storage_index;
    bool updatable;
    
    v3 position;
    v3 velocity_d;
    
    f32 distance_limit;
    
    Entity_type type;
    u32 flags;
    
    Sim_entity_collision_group* collision;
    
    i32 d_abs_tile_z;
    
    u32 hit_points_max;
    Hit_point hit_point[16];
    
    u32 facing_direction;
    f32 t_bob;
    
    Entity_ref sword;
    
    v2 walkable_dim;
    f32 walkable_height;
};

struct Sim_entity_hash {
    Sim_entity* pointer;
    u32 index;
};

struct Sim_region {
    World* world;
    
    f32 max_entity_radius;
    f32 max_entity_velocity;
    
    World_position origin;
    
    Rect3 bounds;
    Rect3 updatable_bounds;
    
    u32 entity_count;
    u32 max_entity_count;
    
    Sim_entity* entity_list;
    
    Sim_entity_hash hash[4000];
};

#endif //SIM_REGION_H
