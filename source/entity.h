/* date = November 11th 2021 5:33 pm */

#ifndef ENTITY_H
#define ENTITY_H

static const v3 Global_invalid_pos = v3{100000.0f, 100000.0f, 100000.0f};

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

static
f32
get_stair_ground(Sim_entity* entity, v3 ground_point) {
  
  assert(entity->type == Entity_type_stairwell);
  
  Rect2 region_rect = rect_center_dim(entity->position.xy, entity->walkable_dim);
  
  v2 unclamped_bary = get_barycentric(region_rect, ground_point.xy);
  v2 barycentric = clamp01(unclamped_bary);
  
  f32 result = entity->position.z + barycentric.y * entity->walkable_height;
  
  return result;
}

static
v3
get_entity_ground_point(Sim_entity* entity, v3 for_entity_pos) {
  
  v3 result = for_entity_pos;
  
  return result;
}

static
v3
get_entity_ground_point(Sim_entity* entity) {
  
  v3 result = get_entity_ground_point(entity, entity->position);
  
  return result;
}


#endif //ENTITY_H
