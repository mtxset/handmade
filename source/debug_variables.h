/* date = October 30th 2025 6:27 pm */

#ifndef DEBUG_VARIABLES_H
#define DEBUG_VARIABLES_H

struct Debug_var_defintion_context {
  Debug_state *state;
  Memory_arena *arena;
  
  Debug_var *group;
};

static
Debug_var*
debug_add_var(Debug_var_defintion_context* ctx, Debug_var_type type, char *name) {
  
  Debug_var *var = mem_push_struct(ctx->arena, Debug_var);
  
  i32 var_length = string_len(name);
  
  var->type = type;
  var->name = (char*)mem_push_copy(ctx->arena, var_length + 1, name);
  var->next = 0;
  
  Debug_var *group = ctx->group;
  var->parent = group;
  
  if (group) {
    
    if (group->group.last_child) {
      group->group.last_child = group->group.last_child->next = var;
    }
    else {
      group->group.last_child = group->group.first_child = var;
    }
    
  }
  
  return var;
}

static
Debug_var*
debug_begin_variable_group(Debug_var_defintion_context *ctx, char *name) {
  Debug_var *group = debug_add_var(ctx, Debug_var_type_group, name);
  group->group.expanded = false;
  group->group.first_child = group->group.last_child = 0;
  
  ctx->group = group;
  
  return group;
}

static
Debug_var*
debug_add_var(Debug_var_defintion_context* ctx, char *name, bool value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_bool, name);
  var->bool_val = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_defintion_context* ctx, char *name, i32 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_i32, name);
  var->int32 = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_defintion_context* ctx, char *name, f32 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_f32, name);
  var->float32 = value;
  
  return var;
}


static
Debug_var*
debug_add_var(Debug_var_defintion_context* ctx, char *name, v4 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_v4, name);
  var->vec4 = value;
  
  return var;
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
      debug_variable_listing(use_debug_camera);
      debug_variable_listing(debug_camera_distance);
      debug_variable_listing(use_room_based_camera);
      debug_end_variable_group(ctx);
    }
    
    debug_end_variable_group(ctx);
  }
  
  debug_variable_listing(familiar_follows_hero);
  debug_variable_listing(use_space_outlines);
  debug_variable_listing(faux_v4);
  
#undef debug_variable_listing
}

#endif //DEBUG_VARIABLES_H
