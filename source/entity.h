/* date = November 11th 2021 5:33 pm */

#ifndef ENTITY_H
#define ENTITY_H

global_var const v3 Global_invalid_pos = v3{100000.0f, 100000.0f, 100000.0f};

inline
bool 
is_set(Sim_entity* entity, u32 sim_entity_flag) {
    bool result = entity->flags & sim_entity_flag;
    return result;
}

inline
void 
add_flags(Sim_entity* entity, u32 sim_entity_flag) {
    entity->flags |= sim_entity_flag;
}

inline
void 
clear_flags(Sim_entity* entity, u32 sim_entity_flag) {
    entity->flags &= ~sim_entity_flag;
}

inline
void make_entity_non_spatial(Sim_entity* entity) {
    add_flags(entity, Entity_flag_non_spatial);
    entity->position = Global_invalid_pos;
}

inline
void 
make_entity_spatial(Sim_entity* entity, v3 pos, v3 velocity_d) {
    clear_flags(entity, Entity_flag_non_spatial);
    entity->position = pos;
    entity->velocity_d = velocity_d;
}

#endif //ENTITY_H
