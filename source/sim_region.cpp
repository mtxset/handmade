#include "sim_region.h"
#include "world.h"

inline 
void 
store_entity_ref(Entity_ref* ref) {
    if (ref->pointer != 0) {
        ref->index = ref->pointer->storage_index;
    }
}

internal
Sim_entity_hash* 
get_hash_from_storage_index(Sim_region* sim_region, u32 storage_index) {
    macro_assert(storage_index);
    
    Sim_entity_hash* result = 0;
    
    u32 hash_value = storage_index;
    for (u32 offset = 0; offset < macro_array_count(sim_region->hash); offset++) {
        // look for first free entry
        u32 hash_mask = macro_array_count(sim_region->hash) - 1;
        u32 hash_index = (hash_value + offset) & hash_mask;
        Sim_entity_hash* entry = sim_region->hash + hash_index;
        
        if (entry->index == 0 || entry->index == storage_index) {
            result = entry;
            break;
        }
    }
    
    return result;
}

internal
void
map_storage_index_to_entity(Sim_region* sim_region, u32 storage_index, Sim_entity* entity) {
    Sim_entity_hash* entry = get_hash_from_storage_index(sim_region, storage_index);
    macro_assert(entry->index == 0 || entry->index == storage_index);
    entry->index = storage_index;
    entry->pointer = entity;
}

inline
Sim_entity* get_entity_by_storage_index(Sim_region* sim_region, u32 storage_index) {
    Sim_entity_hash* entry = get_hash_from_storage_index(sim_region, storage_index);
    Sim_entity* result = entry->pointer;
    
    return result;
}

inline
v2 get_sim_space_pos(Sim_region* sim_region, Low_entity* entity) {
    v2 result = { 100000.0f, 100000.0f };
    
    if (!is_set(&entity->sim, Entity_flag_non_spatial)) {
        World_diff diff = subtract_pos(sim_region->world, &entity->position, &sim_region->origin);
        result = diff.xy;
    }
    
    return result;
}

internal
Sim_entity* 
add_entity(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source, v2* simulation_pos);

inline
void 
load_entity_ref(Game_state* game_state, Sim_region* sim_region, Entity_ref* ref) {
    
    if (!ref->index)
        return;
    
    Sim_entity_hash* entry = get_hash_from_storage_index(sim_region, ref->index);
    
    if (entry->pointer == 0) {
        entry->index = ref->index;
        Low_entity* low_entity = get_low_entity(game_state, ref->index);
        v2 pos = get_sim_space_pos(sim_region, low_entity);
        entry->pointer = add_entity(game_state, sim_region, ref->index, low_entity, &pos);
    }
    
    ref->pointer = entry->pointer;
}

internal
Sim_entity* 
add_entity_raw(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source) {
    
    macro_assert(storage_index);
    Sim_entity* entity = 0;
    
    Sim_entity_hash* entry = get_hash_from_storage_index(sim_region, storage_index);
    
    if (entry->pointer == 0) {
        if (sim_region->entity_count < sim_region->max_entity_count) {
            
            entity = sim_region->entity_list + sim_region->entity_count++;
            
            entry->index = storage_index;
            entry->pointer = entity;
            
            if (source) {
                *entity = source->sim;
                load_entity_ref(game_state, sim_region, &entity->sword);
                
                macro_assert(!is_set(&source->sim, Entity_flag_simming));
                add_flag(&source->sim, Entity_flag_simming);
            }
            
            entity->storage_index = storage_index;
            entity->updatable = false;
        }
        else {
            macro_assert(!"INVALID");
        }
    }
    
    return entity;
}

internal
Sim_entity* 
add_entity(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source, v2* simulation_pos) {
    
    Sim_entity* dest = add_entity_raw(game_state, sim_region, storage_index, source);
    
    if (dest) {
        if (simulation_pos) {
            dest->position = *simulation_pos;
            dest->updatable = is_in_rect(sim_region->updatable_bounds, dest->position);
        }
        else
            dest->position = get_sim_space_pos(sim_region, source);
    }
    
    return dest;
}

internal
Sim_region*
begin_sim(Memory_arena* sim_arena, Game_state* game_state, World* world, World_position origin, Rect2 bounds) {
    
    Sim_region* sim_region = mem_push_struct(sim_arena, Sim_region);
    mem_zero_struct(sim_region->hash);
    
    f32 update_safety_margin = 1.0f;
    
    sim_region->world  = world;
    sim_region->origin = origin;
    sim_region->updatable_bounds = bounds;
    sim_region->bounds = add_radius_to(sim_region->updatable_bounds, update_safety_margin, update_safety_margin);
    
    sim_region->max_entity_count = 4096;
    sim_region->entity_count = 0;
    sim_region->entity_list = mem_push_array(sim_arena, sim_region->max_entity_count, Sim_entity);
    
    World_position min_chunk_pos = map_into_chunk_space(world, sim_region->origin, get_min_corner(sim_region->bounds));
    World_position max_chunk_pos = map_into_chunk_space(world, sim_region->origin, get_max_corner(sim_region->bounds));
    
    for (i32 chunk_y = min_chunk_pos.chunk_y; chunk_y <= max_chunk_pos.chunk_y; chunk_y++) {
        for (i32 chunk_x = min_chunk_pos.chunk_x; chunk_x <= max_chunk_pos.chunk_x; chunk_x++) {
            
            World_chunk* chunk = get_world_chunk(world, chunk_x, chunk_y, sim_region->origin.chunk_z);
            
            if (!chunk)
                continue;
            
            for (World_entity_block* block = &chunk->first_block; block; block = block->next) {
                for (u32 entity_index = 0; entity_index < block->entity_count; entity_index++) {
                    u32 low_entity_index = block->low_entity_index[entity_index];
                    Low_entity* low_entity = game_state->low_entity_list + low_entity_index;
                    
                    if (is_set(&low_entity->sim, Entity_flag_non_spatial))
                        continue;
                    
                    v2 sim_space_pos = get_sim_space_pos(sim_region, low_entity);
                    if (is_in_rect(sim_region->bounds, sim_space_pos))
                        add_entity(game_state, sim_region, low_entity_index, low_entity, &sim_space_pos);
                }
            }
        }
    }
    return sim_region;
}

internal
void 
end_sim(Sim_region* region, Game_state* game_state) {
    
    Sim_entity* entity = region->entity_list;
    for (u32 entity_index = 0; entity_index < region->entity_count; entity_index++, entity++) {
        
        Low_entity* stored = game_state->low_entity_list + entity->storage_index;
        
        macro_assert(is_set(&stored->sim, Entity_flag_simming));
        stored->sim = *entity;
        macro_assert(!is_set(&stored->sim, Entity_flag_simming));
        
        store_entity_ref(&stored->sim.sword);
        
        World_position new_pos = is_set(entity, Entity_flag_non_spatial) ? 
            null_position() : map_into_chunk_space(game_state->world, region->origin, entity->position);
        
        change_entity_location(&game_state->world_arena, game_state->world, entity->storage_index, stored, new_pos);
        
        if (entity->storage_index == game_state->following_entity_index) {
            
            World_position new_camera_pos = game_state->camera_pos;
            new_camera_pos.chunk_z = stored->position.chunk_z;
#if 0
            if (camera_follow_entity.high->position.x > 8.5 * world->tile_side_meters)
                new_camera_pos.absolute_tile_x += 17;
            
            if (camera_follow_entity.high->position.x < -(8.5f * world->tile_side_meters))
                new_camera_pos.absolute_tile_x -= 17;
            
            if (camera_follow_entity.high->position.y > 4.5f * world->tile_side_meters)
                new_camera_pos.absolute_tile_y += 9;
            
            if (camera_follow_entity.high->position.y < -(5.0f * world->tile_side_meters))
                new_camera_pos.absolute_tile_y -= 9;
#else
            // smooth follow
            new_camera_pos = stored->position;
#endif
            
            game_state->camera_pos = new_camera_pos;
        }
    }
}

internal
bool test_wall(f32 wall_x, f32 rel_x, f32 rel_y, f32 player_delta_x, f32 player_delta_y, f32* t_min, f32 min_y, f32 max_y) {
    
    bool hit = false;
    
    f32 t_epsilon = 0.0001f;
    
    if (player_delta_x != 0.0f) {
        f32 t_result = (wall_x - rel_x) / player_delta_x;
        f32 y = rel_y + t_result * player_delta_y;
        
        if (t_result >= 0.0f && *t_min > t_result) {
            if (y >= min_y && y <= max_y) {
                *t_min = max(0.0f, t_result - t_epsilon);
                hit = true;
            }
        }
    }
    
    return hit;
}

internal
bool
handle_collision(Sim_entity* a_entity, Sim_entity* b_entity) {
    bool stops_on_collision = false;
    
    if (a_entity->type == Entity_type_sword) {
        stops_on_collision = false;
    }
    else {
        stops_on_collision = true;
    }
    
    if (a_entity->type > b_entity->type) {
        swap(a_entity, b_entity);
    }
    
    if (a_entity->type == Entity_type_monster && b_entity->type == Entity_type_sword) {
        if (a_entity->hit_points_max > 0)
            a_entity->hit_points_max--;
    }
    
    // entity->abs_tile_z += hit_low->sim.d_abs_tile_z;
    
    return stops_on_collision;
}

internal
void
clear_collision_rule(Game_state* game_state, u32 storage_index) {
    for (u32 hash_bucket = 0; hash_bucket < macro_array_count(game_state->collision_rule_hash); hash_bucket++) {
        for (Pairwise_collision_rule** rule = & game_state->collision_rule_hash[hash_bucket]; *rule;) {
            
            if ((*rule)->storage_index_a == storage_index || 
                (*rule)->storage_index_b == storage_index) {
                Pairwise_collision_rule* removed_rule = *rule;
                *rule = (*rule)->next_in_hash;
                
                removed_rule->next_in_hash = game_state->first_free_collsion_rule;
                game_state->first_free_collsion_rule = removed_rule;
            }
            else {
                rule = &(*rule)->next_in_hash;
            }
            
        }
    }
}

internal
void
add_collision_rule(Game_state* game_state, u32 storage_index_a, u32 storage_index_b, bool should_collide) {
    
    if (storage_index_a > storage_index_b) {
        swap(storage_index_a, storage_index_b);
    }
    
    Pairwise_collision_rule* found = 0;
    u32 hash_bucket = storage_index_a & (macro_array_count(game_state->collision_rule_hash) - 1);
    for (Pairwise_collision_rule* rule = game_state->collision_rule_hash[hash_bucket]; rule; rule = rule->next_in_hash) {
        if (rule->storage_index_a == storage_index_a &&
            rule->storage_index_b == storage_index_b) {
            found = rule;
            break;
        }
    }
    
    if (!found) {
        found = game_state->first_free_collsion_rule;
        
        if (found) {
            game_state->first_free_collsion_rule = found->next_in_hash;
        }
        else {
            found = mem_push_struct(&game_state->world_arena, Pairwise_collision_rule);
        }
        
        found = mem_push_struct(&game_state->world_arena, Pairwise_collision_rule);
        found->next_in_hash = game_state->collision_rule_hash[hash_bucket];
        game_state->collision_rule_hash[hash_bucket] = found;
    }
    
    if (found) {
        found->storage_index_a = storage_index_a;
        found->storage_index_b = storage_index_b;
        found->should_collide = should_collide;
    }
}

internal
bool
should_collide(Game_state* game_state, Sim_entity* a_entity, Sim_entity* b_entity) {
    bool result = false;
    
    if (a_entity->storage_index > b_entity->storage_index) {
        swap(a_entity, b_entity);
    }
    
    if (!is_set(a_entity, Entity_flag_non_spatial) &&
        !is_set(b_entity, Entity_flag_non_spatial)) {
        result = true;
    }
    
    u32 hash_bucket = a_entity->storage_index & (macro_array_count(game_state->collision_rule_hash) - 1);
    for (Pairwise_collision_rule* rule = game_state->collision_rule_hash[hash_bucket]; rule; rule = rule->next_in_hash) {
        if (rule->storage_index_a == a_entity->storage_index &&
            rule->storage_index_b == b_entity->storage_index) {
            result = rule->should_collide;
            break;
        }
    }
    
    return result;
}

internal
void
move_entity(Game_state* game_state, Sim_region* sim_region, Sim_entity* entity, f32 time_delta, Move_spec* move_spec, v2 player_acceleration_dd) {
    
    macro_assert(!is_set(entity, Entity_flag_non_spatial));
    
    World* world = sim_region->world;
    
    if (move_spec->max_acceleration_vector) {
        f32 accelecation_dd_length = length_squared_v2(player_acceleration_dd);
        
        if (accelecation_dd_length > 1.0f) {
            player_acceleration_dd *= 1.0f / square_root(accelecation_dd_length);
        }
    }
    
    f32 move_speed = move_spec->speed;
    
    if (move_spec->boost)
        move_speed *= move_spec->boost;
    
    player_acceleration_dd *= move_speed;
    player_acceleration_dd += -move_spec->drag * entity->velocity_d;
    
    v2 old_player_pos = entity->position;
    
    // p' = (1/2) * at^2 + vt + ..
    v2 player_delta = 0.5f * player_acceleration_dd * square(time_delta) + entity->velocity_d * time_delta;
    
    // v' = at + v
    entity->velocity_d = player_acceleration_dd * time_delta + entity->velocity_d;
    
    // p' = ... + p
    v2 new_player_pos = old_player_pos + player_delta;
    
    f32 distance_remaining = entity->distance_limit;
    if (distance_remaining == 0.0f) {
        // set to no limit
        distance_remaining = 9999.0f;
    }
    
    u32 correction_tries = 4;
    for (u32 i = 0; i < correction_tries; i++) {
        
        f32 t_min = 1.0f;
        
        f32 player_delta_length = length_v2(player_delta);
        
        if (player_delta_length <= 0.0f)
            break;
        
        if (player_delta_length > distance_remaining) {
            t_min = distance_remaining / player_delta_length;
        }
        
        v2 wall_normal = { };
        Sim_entity* hit_entity = 0;
        
        v2 desired_postion = entity->position + player_delta;
        
        if (!is_set(entity, Entity_flag_non_spatial)) {
            
            for (u32 test_high_entity_index = 0; test_high_entity_index < sim_region->entity_count; test_high_entity_index++) {
                
                Sim_entity* test_entity =  sim_region->entity_list + test_high_entity_index;
                // skip self collission
                if (!should_collide(game_state, entity, test_entity))
                    continue;
                
                f32 diameter_width  = test_entity->width  + entity->width;
                f32 diameter_height = test_entity->height + entity->height;
                // assumption: compiler knows these values (min/max_coner) are not using anything from loop so it can pull it out
                v2 min_corner = -0.5f * v2 { diameter_width, diameter_height };
                v2 max_corner =  0.5f * v2 { diameter_width, diameter_height };
                
                // relative position delta
                v2 rel = entity->position - test_entity->position;
                
                if (test_wall(min_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y)) {
                    wall_normal = {-1, 0 };
                    hit_entity = test_entity;
                }
                
                if (test_wall(max_corner.x, rel.x, rel.y, player_delta.x, player_delta.y, &t_min, min_corner.y, max_corner.y)) {
                    wall_normal = { 1, 0 };
                    hit_entity = test_entity;
                }
                
                if (test_wall(min_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x)) {
                    wall_normal = { 0, -1 };
                    hit_entity = test_entity;
                }
                
                if (test_wall(max_corner.y, rel.y, rel.x, player_delta.y, player_delta.x, &t_min, min_corner.x, max_corner.x)) {
                    wall_normal = { 0, 1 };
                    hit_entity = test_entity;
                }
            }
        }
        
        entity->position += t_min * player_delta;
        distance_remaining -= t_min * player_delta_length;
        
        if (hit_entity) {
            //v' = v - 2*dot(v,r) * r
            v2 r = wall_normal;
            v2 v = entity->velocity_d;
            // game_state->player_velocity_d = v - 2 * inner(v, r) * r; // reflection
            player_delta = desired_postion - entity->position;
            
            bool stops_on_collision = handle_collision(entity, hit_entity);
            
            if (stops_on_collision) {
                entity->velocity_d = v - 1 * inner(v, r) * r;
                player_delta = player_delta - 1 * inner(player_delta, r) * r;
            }
            else {
                add_collision_rule(game_state, entity->storage_index, hit_entity->storage_index, false);
            }
        }
        else {
            break;
        }
    }
    
    if (entity->distance_limit != 0.0f) {
        entity->distance_limit = distance_remaining;
    }
    
    if (entity->velocity_d.x == 0 && entity->velocity_d.y == 0) {
        
    }
    else if (absolute(entity->velocity_d.x) > absolute(entity->velocity_d.y)) {
        if (entity->velocity_d.x > 0)
            entity->facing_direction = 0;
        else
            entity->facing_direction = 2;
    }
    else {
        if (entity->velocity_d.y > 0)
            entity->facing_direction = 1;
        else
            entity->facing_direction = 3;
    }
    
#if 0
    char buffer[256];
    _snprintf_s(buffer, sizeof(buffer), "tile(x,y): %u, %u; relative(x,y): %f, %f\n", 
                entity.position.absolute_tile_x, entity.position.absolute_tile_y,
                entity->position.x, entity->position.y);
    OutputDebugStringA(buffer);
#endif
}