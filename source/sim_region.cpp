#include "sim_region.h"
#include "world.h"

inline
v2 get_sim_space_pos(Sim_region* sim_region, Low_entity* entity) {
    v2 result = {};
    
    World_diff diff = subtract_pos(sim_region->world, &entity->position, &sim_region->origin);
    result = diff.xy;
    
    return result;
}

internal
Sim_entity* add_entity(Sim_region* sim_region) {
    Sim_entity* entity = 0;
    
    if (sim_region->entity_count < sim_region->max_entity_count) {
        entity = sim_region->entity_list + sim_region->entity_count++;
        
        entity = {};
    }
    else {
        macro_assert(!"INVALID");
    }
    
    return entity;
}


internal
Sim_entity* add_entity(Sim_region* sim_region, Low_entity* source, v2* simulation_pos) {
    Sim_entity* dest = add_entity(sim_region);
    
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
                        add_entity(sim_region, low_entity, &sim_space_pos);
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
        
        World_position new_pos = map_into_chunk_space(game_state->world, region->origin, entity->position);
        
        change_entity_location(&game_state->world_arena, game_state->world, 
                               entity->storage_index, stored, 
                               &stored->position, &new_pos);
        
        Entity camera_follow_entity = force_entity_into_high(game_state, game_state->following_entity_index);
        
        if (camera_follow_entity.high) {
            
            World_position new_camera_pos = game_state->camera_pos;
            new_camera_pos.chunk_z = camera_follow_entity.low->position.chunk_z;
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
            new_camera_pos = camera_follow_entity.low->position;
#endif
            set_camera(game_state, new_camera_pos);
        }
    }
}