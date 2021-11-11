
inline
Move_spec default_move_spec() {
    Move_spec result;
    
    result.max_acceleration_vector = false;
    result.speed = 1.0f;
    result.drag = 0.0f;
    result.boost = 0.0f;
    
    return result;
}

// https://youtu.be/KBCNjjeXezM
inline
void update_familiar(Sim_region* sim_region, Sim_entity* entity, f32 time_delta) {
    Sim_entity* closest_hero = 0;
    // distance we want it start to follow
    f32 closest_hero_delta_squared = square(10.0f);
    
    Sim_entity* test_entity = sim_region->entity_list;
    for (u32 entity_index = 0; entity_index < sim_region->entity_count; entity_index++) {
        if (test_entity->type != Entity_type_hero)
            continue;
        
        f32 test_delta_squared = length_squared_v2(test_entity->position - entity->position);
        
        // giving some priority for hero
        if (test_entity->type == Entity_type_hero) {
            test_delta_squared *= 0.75f;
        }
        
        if (closest_hero_delta_squared > test_delta_squared) {
            closest_hero = test_entity;
            closest_hero_delta_squared = test_delta_squared;
        }
    }
    
    v2 velocity = {};
    f32 distance_to_stop = 2.5f;
    bool move_closer = closest_hero_delta_squared > square(distance_to_stop);
    if (closest_hero && move_closer) {
        f32 acceleration = 0.5f;
        f32 one_over_length = acceleration / square_root(closest_hero_delta_squared);
        velocity = one_over_length * (closest_hero->position - entity->position);
    }
    
    Move_spec move_spec = default_move_spec();
    move_spec.max_acceleration_vector = true;
    move_spec.speed = 50.0f;
    move_spec.drag = 8.0f;
    move_spec.boost = 1.0f;
    
    move_entity(sim_region, entity, time_delta, &move_spec, velocity);
}

inline
void update_sword(Sim_region* sim_region, Sim_entity* entity, f32 time_delta) {
    Move_spec move_spec = {};
    move_spec.max_acceleration_vector = false;
    move_spec.speed = 0.0f;
    move_spec.drag = 0.0f;
    move_spec.boost = 0.0f;
    
    v2 old_pos = entity->position;
    move_entity(sim_region, entity, time_delta, &move_spec, v2 {0, 0});
    
    f32 distance_travelled = length_v2(entity->position - old_pos);
    entity->sword_distance_remaining -= distance_travelled;
    
    if (entity->sword_distance_remaining < 0.0f) {
        macro_assert(!"NEED TO MAKE ENTITIES BE ABLE TO NOT BE THERE!");
    }
}

inline
void update_monster(Sim_region* sim_region, Sim_entity* entity, f32 time_delta) {
    
}

