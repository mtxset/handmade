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
  
  var->type = type;
  var->name = name;
  var->next = 0;
  
  Debug_var *group = ctx->group;
  var->parent = group;
  
  if (group) {
    
    if (group->group.last_child) {
      group->group.last_child = group->group.last_child->next = var;
    }
    else {
      group->group.last_child = group->group.first_child->next = var;
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
  Debug_var *var = debug_add_var(ctx, Debug_var_type_group, name);
  var->bool_val = value;
  
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
debug_create_vars(Debug_state *state) {
  
  Debug_var_defintion_context ctx = {};
  ctx.state = state;
  ctx.arena = &state->debug_arena;
  ctx.group = debug_begin_variable_group(&ctx, "root");
  
#define debug_variable_listing(name) Debug_var_type_bool, #name, name
  
  debug_begin_variable_group(&ctx, "chunks");
  
  debug_variable_listing(DEBUG_UI_ground_chunks_outlines);
  debug_variable_listing(DEBUG_UI_ground_chunk_checker_board);
  debug_variable_listing(DEBUG_UI_redo_ground_chunks_on_recompile);
  
  debug_end_variable_group(&ctx);
  
#undef debug_variable_listing
  
  state->root_group = ctx.group;
}

/*

Debug_var debug_var_list[] = {
  debug_variable_listing(DEBUG_UI_use_debug_camera),
  debug_variable_listing(DEBUG_UI_particle_test),
  debug_variable_listing(DEBUG_UI_particle_grid),
  debug_variable_listing(DEBUG_UI_use_space_outlines),
  
  
  debug_variable_listing(DEBUG_UI_test_weird_draw_buffer_size),
  debug_variable_listing(DEBUG_UI_familiar_follows_hero),
  debug_variable_listing(DEBUG_UI_show_lighting_samples),
  debug_variable_listing(DEBUG_UI_use_room_based_camera),
};
*/

#endif //DEBUG_VARIABLES_H
