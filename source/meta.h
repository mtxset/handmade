/* date = November 10th 2025 9:13 pm */

#ifndef META_H
#define META_H

enum Meta_type {
  Meta_type_Sorld_chunk,
  Meta_type_u32,
  Meta_type_bool,
  Meta_type_v2,
  Meta_type_v3,
  Meta_type_f32,
  Meta_type_i32,
  Meta_type_Rect2,
  Meta_type_Rect3,
  
  
  Meta_type_Hit_point,
  Meta_type_Entity_type,
  Meta_type_Entity_ref,
  
  Meta_type_World,
  Meta_type_World_position,
  
  Meta_type_Sim_entity_collision_volume_group,
  Meta_type_Sim_region,
  Meta_type_Sim_entity,
  Meta_type_Sim_entity_hash,
  Meta_type_Sim_entity_collision_volume,
};

enum Member_definition_flag {
  Meta_member_flag_is_pointer = 0x1,
};

struct Member_definition {
  u32 flags;
  Meta_type type;
  char *name;
  u32 offset;
};


#endif //META_H
