#include "sim_region.h"
#include "world.h"

inline void
store_entity_ref(Entity_ref* ref) {
    if (ref->pointer != 0) {
        ref->index = ref->pointer->storage_index;
    }
}

internal
Sim_entity_hash* get_hash_from_storage_index(Sim_region* sim_region, u32 storage_index) {
    macro_assert(storage_index);
    
    Sim_entity_hash* result = 0;
    
    u32 hash_value = storage_index;
    for (u32 offset = 0; offset < macro_array_count(sim_region->hash); offset++) {
        // look for first free entry
        Sim_entity_hash* entry = sim_region->hash + ((hash_value + offset) & (macro_array_count(sim_region->hash) - 1));
        
        if (entry->index == 0 || entry->index == storage_index) {
            result = entry;
            break;
        }
    }
    
    return result;
}

internal
void map_storage_index_to_entity(Sim_region* sim_region, u32 storage_index, Sim_entity* entity) {
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
    v2 result = {};
    
    World_diff diff = subtract_pos(sim_region->world, &entity->position, &sim_region->origin);
    result = diff.xy;
    
    return result;
}

internal
Sim_entity* add_entity(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source);
inline
void load_entity_ref(Game_state* game_state, Sim_region* sim_region, Entity_ref* ref) {
    
    if (!ref->index)
        return;
    
    Sim_entity_hash* entry = get_hash_from_storage_index(sim_region, ref->index);
    
    if (entry->pointer == 0) {
        entry->index = ref->index;
        Low_entity* low_entity = get_low_entity(game_state, ref->index);
        entry->pointer = add_entity(game_state, sim_region, ref->index, low_entity);
    }
    
    ref->pointer = entry->pointer;
}

internal
Sim_entity* add_entity(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source) {
    
    macro_assert(storage_index);
    Sim_entity* entity = 0;
    
    if (sim_region->entity_count < sim_region->max_entity_count) {
        entity = sim_region->entity_list + sim_region->entity_count++;
        map_storage_index_to_entity(sim_region, storage_index, entity);
        
        if (source) {
            *entity = source->sim;
            load_entity_ref(game_state, sim_region, &entity->sword);
        }
        
        entity->storage_index = storage_index;
    }
    else {
        macro_assert(!"INVALID");
    }
    
    return entity;
}

internal
Sim_entity* add_entity(Game_state* game_state, Sim_region* sim_region, u32 storage_index, Low_entity* source, v2* simulation_pos) {
    
    Sim_entity* dest = add_entity(game_state, sim_region, storage_index, source);
    if (dest) {
        if (simulation_pos)
            dest->position = *simulation_pos;
        else
            dest->position = get_sim_space_pos(sim_region, source);
    }
    
}

internal
Sim_region* begin_sim(Memory_arena* sim_arena, Game_state* game_state, World* world, World_position origin, Rect2 bounds) {
    
    Sim_region* sim_region = mem_push_struct(sim_arena, Sim_region);
    
    sim_region->world  = world;
    sim_region->origin = origin;
    sim_region->bounds = bounds;
    
    sim_region->max_entity_count = 1024;
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
                    
                    v2 sim_space_pos = get_sim_space_pos(sim_region, low_entity);
                    if (is_in_rect(sim_region->bounds, sim_space_pos))
                        add_entity(game_state, sim_region, low_entity_index, low_entity, &sim_space_pos);
                }
            }
        }
    }
    
}

internal
void end_sim(Sim_region* region, Game_state* game_state) {
    Sim_entity* entity = region->entity_list;
    
    for (u32 entity_index = 0; entity_index < region->entity_count; entity_index++, entity++) {
        Low_entity* stored = game_state->low_entity_list + entity->storage_index;
        
        stored->sim = *entity;
        store_entity_ref(&stored->sim.sword);
        
        World_position new_pos = map_into_chunk_space(game_state->world, region->origin, entity->position);
        
        change_entity_location(&game_state->world_arena, game_state->world, 
                               entity->storage_index, stored, 
                               &stored->position, &new_pos);
        
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
        }
    }
}

internal
bool test_wall(f32 wall_x, f32 rel_x, f32 rel_y, f32 player_delta_x, f32 player_delta_y, f32* t_min, f32 min_y, f32 max_y) {
    
    bool hit = false;
    
    f32 t_epsilon = 0.0001f;
    
    if (player_delta_x == 0.0f)
        return hit;
    
    f32 t_result = (wall_x - rel_x) / player_delta_x;
    f32 y = rel_y + t_result * player_delta_y;
    
    if (t_result >= 0.0f && *t_min > t_result) {
        if (y >= min_y && y <= max_y) {
            *t_min = max(0.0f, t_result - t_epsilon);
            hit = true;
        }
    }
    
    return hit;
}

internal
void move_entity(Sim_region* sim_region, Sim_entity* entity, f32 time_delta, Move_spec* move_spec, v2 player_acceleration_dd) {
    
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
    
    u32 correction_tries = 4;
    for (u32 i = 0; i < correction_tries; i++) {
        f32 t_min = 1.0f;
        v2 wall_normal = { };
        Sim_entity* hit_entity = 0;
        
        v2 desired_postion = entity->position + player_delta;
        
        if (entity->collides) {
            
            // skipping 0 entity
            for (u32 test_high_entity_index = 0; test_high_entity_index < sim_region->entity_count; test_high_entity_index++) {
                
                Sim_entity* test_entity =  sim_region->entity_list + test_high_entity_index;
                // skip self collission
                if (entity == test_entity)
                    continue;
                
                if (!test_entity->collides)
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
        if (hit_entity) {
            //v' = v - 2*dot(v,r) * r
            v2 r = wall_normal;
            v2 v = entity->velocity_d;
            // game_state->player_velocity_d = v - 2 * inner(v, r) * r; // reflection
            entity->velocity_d = v - 1 * inner(v, r) * r;
            player_delta = desired_postion - entity->position;
            player_delta = player_delta - 1 * inner(player_delta, r) * r;
            
            // entity->abs_tile_z += hit_low->sim.d_abs_tile_z;
        }
        else {
            break;
        }
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