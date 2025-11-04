/* date = October 30th 2025 6:27 pm */

#ifndef DEBUG_VARIABLES_H
#define DEBUG_VARIABLES_H

struct Debug_var_defintion_context {
  Debug_state *state;
  Memory_arena *arena;
  
  Debug_var_reference *group;
};

static
Debug_var*
debug_add_unrefrenced_var(Debug_state *debug_state, Debug_var_type type, char *name) {
  
  Debug_var *var = mem_push_struct(&debug_state->debug_arena, Debug_var);
  
  i32 str_length = string_len(name);
  var->type = type;
  var->name = (char*)mem_push_copy(&debug_state->debug_arena, str_length + 1, name);
  
  return var;
}

static
Debug_var_reference*
debug_add_var_reference(Debug_state *debug_state, Debug_var_reference *group_ref, Debug_var *var) {
  
  Debug_var_reference* ref = mem_push_struct(&debug_state->debug_arena, Debug_var_reference);
  ref->var = var;
  ref->next = 0;
  
  ref->parent = group_ref;
  Debug_var *group = (ref->parent) ? ref->parent->var : 0;
  
  if (group) {
    
    if (group->group.last_child) {
      group->group.last_child = group->group.last_child->next = ref;
    }
    else {
      group->group.last_child = group->group.first_child = ref;
    }
    
  }
  
  return ref;
}

static
Debug_var_reference*
debug_add_var_reference(Debug_var_defintion_context *ctx, Debug_var *var) {
  Debug_var_reference *ref = debug_add_var_reference(ctx->state, ctx->group, var);
  
  return ref;
}

static
Debug_var_reference*
debug_add_var(Debug_var_defintion_context *ctx, Debug_var_type type, char *name) {
  
  Debug_var *var = debug_add_unrefrenced_var(ctx->state, type, name);
  Debug_var_reference *var_ref = debug_add_var_reference(ctx, var);
  
  return var_ref;
}

static 
Debug_var*
debug_add_root_group_internal(Debug_state *state, char *name) {
  Debug_var *group = debug_add_unrefrenced_var(state, Debug_var_type_group, name);
  
  group->group.expanded = true;
  group->group.first_child = group->group.last_child = 0;
  
  return group;
}

static 
Debug_var_reference*
debug_add_root_group(Debug_state *state, char *name) {
  Debug_var_reference *group_ref = debug_add_var_reference(state, 0, debug_add_root_group_internal(state, name));
  
  return group_ref;
}

static
Debug_var_reference*
debug_begin_variable_group(Debug_var_defintion_context *ctx, char *name) {
  Debug_var_reference *group = debug_add_var_reference(ctx, debug_add_root_group_internal(ctx->state, name));
  group->var->group.expanded = false;
  
  ctx->group = group;
  
  return group;
}

static
Debug_var_reference*
debug_add_var(Debug_var_defintion_context* ctx, char *name, bool value) {
  Debug_var_reference *ref = debug_add_var(ctx, Debug_var_type_bool, name);
  ref->var->bool_val = value;
  
  return ref;
}

static
Debug_var_reference*
debug_add_var(Debug_var_defintion_context* ctx, char *name, i32 value) {
  Debug_var_reference *ref = debug_add_var(ctx, Debug_var_type_i32, name);
  ref->var->int32 = value;
  
  return ref;
}

static
Debug_var_reference*
debug_add_var(Debug_var_defintion_context* ctx, char *name, f32 value) {
  Debug_var_reference *ref = debug_add_var(ctx, Debug_var_type_f32, name);
  ref->var->float32 = value;
  
  return ref;
}

static
Debug_var_reference*
debug_add_var(Debug_var_defintion_context* ctx, char *name, v4 value) {
  Debug_var_reference *ref = debug_add_var(ctx, Debug_var_type_v4, name);
  ref->var->vec4 = value;
  
  return ref;
}

static
void
debug_end_variable_group(Debug_var_defintion_context *ctx) {
  assert(ctx->group);
  
  ctx->group = ctx->group->parent;
}

static
void
debug_create_vars(Debug_var_defintion_context *ctx) {
  
  Debug_var_reference *use_debug_cam_ref = 0;
  
#define debug_variable_listing(name) debug_add_var(ctx, #name, DEBUG_UI_##name);
  
  {
    debug_begin_variable_group(ctx, "chunks");
    debug_variable_listing(ground_chunks_outlines);
    debug_variable_listing(ground_chunk_checker_board);
    debug_variable_listing(redo_ground_chunks_on_recompile);
    debug_end_variable_group(ctx);
  }
  
  {
    debug_begin_variable_group(ctx, "particles");
    debug_variable_listing(particle_test);
    debug_variable_listing(particle_grid);
    debug_end_variable_group(ctx);
  }
  
  {
    debug_begin_variable_group(ctx, "renderer");
    debug_variable_listing(show_lighting_samples);
    debug_variable_listing(test_weird_draw_buffer_size);
    
    {
      debug_begin_variable_group(ctx, "camera");
      use_debug_cam_ref = debug_variable_listing(use_debug_camera);
      debug_variable_listing(debug_camera_distance);
      debug_variable_listing(use_room_based_camera);
      debug_end_variable_group(ctx);
    }
    
    debug_end_variable_group(ctx);
  }
  
  debug_variable_listing(familiar_follows_hero);
  debug_variable_listing(use_space_outlines);
  debug_variable_listing(faux_v4);
  
  debug_add_var_reference(ctx, use_debug_cam_ref->var);
  
#undef debug_variable_listing
}

#endif //DEBUG_VARIABLES_H
