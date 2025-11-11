Member_definition members_of_Sim_entity_collision_volume[] = {
 {0, Meta_type_v3, "offset_pos", (u32)(size_t)&((Sim_entity_collision_volume*)0)->offset_pos},
 {0, Meta_type_v3, "dim", (u32)(size_t)&((Sim_entity_collision_volume*)0)->dim},
};

Member_definition members_of_Sim_entity_collision_volume_group[] = {
 {0, Meta_type_Sim_entity_collision_volume, "total_volume", (u32)(size_t)&((Sim_entity_collision_volume_group*)0)->total_volume},
 {0, Meta_type_u32, "volume_count", (u32)(size_t)&((Sim_entity_collision_volume_group*)0)->volume_count},
 {Meta_member_flag_is_pointer, Meta_type_Sim_entity_collision_volume, "volume_list", (u32)(size_t)&((Sim_entity_collision_volume_group*)0)->volume_list},
};

Member_definition members_of_Sim_entity[] = {
 {0, Meta_type_u32, "storage_index", (u32)(size_t)&((Sim_entity*)0)->storage_index},
 {0, Meta_type_bool, "updatable", (u32)(size_t)&((Sim_entity*)0)->updatable},
 {0, Meta_type_v3, "position", (u32)(size_t)&((Sim_entity*)0)->position},
 {0, Meta_type_v3, "velocity_d", (u32)(size_t)&((Sim_entity*)0)->velocity_d},
 {0, Meta_type_f32, "distance_limit", (u32)(size_t)&((Sim_entity*)0)->distance_limit},
 {0, Meta_type_Entity_type, "type", (u32)(size_t)&((Sim_entity*)0)->type},
 {0, Meta_type_u32, "flags", (u32)(size_t)&((Sim_entity*)0)->flags},
 {Meta_member_flag_is_pointer, Meta_type_Sim_entity_collision_volume_group, "collision", (u32)(size_t)&((Sim_entity*)0)->collision},
 {0, Meta_type_i32, "d_abs_tile_z", (u32)(size_t)&((Sim_entity*)0)->d_abs_tile_z},
 {0, Meta_type_u32, "hit_points_max", (u32)(size_t)&((Sim_entity*)0)->hit_points_max},
 {0, Meta_type_Hit_point, "hit_point", (u32)(size_t)&((Sim_entity*)0)->hit_point},
 {0, Meta_type_f32, "facing_direction", (u32)(size_t)&((Sim_entity*)0)->facing_direction},
 {0, Meta_type_f32, "t_bob", (u32)(size_t)&((Sim_entity*)0)->t_bob},
 {0, Meta_type_Entity_ref, "sword", (u32)(size_t)&((Sim_entity*)0)->sword},
 {0, Meta_type_v2, "walkable_dim", (u32)(size_t)&((Sim_entity*)0)->walkable_dim},
 {0, Meta_type_f32, "walkable_height", (u32)(size_t)&((Sim_entity*)0)->walkable_height},
};

Member_definition members_of_Sim_region[] = {
 {Meta_member_flag_is_pointer, Meta_type_World, "world", (u32)(size_t)&((Sim_region*)0)->world},
 {0, Meta_type_f32, "max_entity_radius", (u32)(size_t)&((Sim_region*)0)->max_entity_radius},
 {0, Meta_type_f32, "max_entity_velocity", (u32)(size_t)&((Sim_region*)0)->max_entity_velocity},
 {0, Meta_type_World_position, "origin", (u32)(size_t)&((Sim_region*)0)->origin},
 {0, Meta_type_Rect3, "bounds", (u32)(size_t)&((Sim_region*)0)->bounds},
 {0, Meta_type_Rect3, "updatable_bounds", (u32)(size_t)&((Sim_region*)0)->updatable_bounds},
 {0, Meta_type_u32, "entity_count", (u32)(size_t)&((Sim_region*)0)->entity_count},
 {0, Meta_type_u32, "max_entity_count", (u32)(size_t)&((Sim_region*)0)->max_entity_count},
 {Meta_member_flag_is_pointer, Meta_type_Sim_entity, "entity_list", (u32)(size_t)&((Sim_region*)0)->entity_list},
 {0, Meta_type_Sim_entity_hash, "hash", (u32)(size_t)&((Sim_region*)0)->hash},
};

Member_definition members_of_Rect2[] = {
 {0, Meta_type_v2, "min", (u32)(size_t)&((Rect2*)0)->min},
 {0, Meta_type_v2, "max", (u32)(size_t)&((Rect2*)0)->max},
};

Member_definition members_of_Rect3[] = {
 {0, Meta_type_v3, "min", (u32)(size_t)&((Rect3*)0)->min},
 {0, Meta_type_v3, "max", (u32)(size_t)&((Rect3*)0)->max},
};

Member_definition members_of_World_position[] = {
 {0, Meta_type_i32, "chunk_x", (u32)(size_t)&((World_position*)0)->chunk_x},
 {0, Meta_type_i32, "chunk_y", (u32)(size_t)&((World_position*)0)->chunk_y},
 {0, Meta_type_i32, "chunk_z", (u32)(size_t)&((World_position*)0)->chunk_z},
 {0, Meta_type_v3, "_offset", (u32)(size_t)&((World_position*)0)->_offset},
};

#define Meta_type_dump(member_ptr, next_ident_level) \
case Meta_type_World_position: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_World_position), members_of_World_position, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Rect3: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Rect3), members_of_Rect3, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Rect2: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Rect2), members_of_Rect2, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Sim_region: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Sim_region), members_of_Sim_region, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Sim_entity: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Sim_entity), members_of_Sim_entity, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Sim_entity_collision_volume_group: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Sim_entity_collision_volume_group), members_of_Sim_entity_collision_volume_group, member_ptr, (next_ident_level)); } break; \
 \
case Meta_type_Sim_entity_collision_volume: { debug_text_line(member->name); debug_dump_struct(array_count(members_of_Sim_entity_collision_volume), members_of_Sim_entity_collision_volume, member_ptr, (next_ident_level)); } break; 
 
