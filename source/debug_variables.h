/* date = October 30th 2025 6:27 pm */

#ifndef DEBUG_VARIABLES_H
#define DEBUG_VARIABLES_H

struct Debug_var_definition_context {
  Debug_state *state;
  Memory_arena *arena;
  
  u32 group_depth;
#define Debug_max_var_stack_depth 64
  Debug_var *group_stack[Debug_max_var_stack_depth];
};

inline
void
debug_list_init(Debug_var_link *sentinel) {
  sentinel->next = sentinel;
  sentinel->prev = sentinel;
}

inline
void
debug_list_insert(Debug_tree *sentinel, Debug_tree *entry) {
  
  entry->next = sentinel->next;
  entry->prev = sentinel;
  
  entry->next->prev = entry;
  entry->prev->next = entry;
}

inline
void
debug_list_insert(Debug_var_link *sentinel, Debug_var_link *entry) {
  
  entry->next = sentinel->next;
  entry->prev = sentinel;
  
  entry->next->prev = entry;
  entry->prev->next = entry;
}

static
Debug_var*
debug_add_var(Debug_state *debug_state, Debug_var_type type, char *name) {
  
  Debug_var *var = mem_push_struct(&debug_state->debug_arena, Debug_var);
  
  i32 str_length = string_len(name);
  var->type = type;
  var->name = (char*)mem_push_copy(&debug_state->debug_arena, str_length + 1, name);
  
  return var;
}

static
void
debug_add_var_to_group(Debug_state *debug_state, Debug_var *group, Debug_var *var) {
  Debug_var_link *link = mem_push_struct(&debug_state->debug_arena, Debug_var_link);
  debug_list_insert(&group->var_group, link);
  link->var = var;
}

static
void
debug_add_var_to_default_group(Debug_var_definition_context *ctx, Debug_var *var) {
  Debug_var *parent = ctx->group_stack[ctx->group_depth];
  
  if (parent) {
    debug_add_var_to_group(ctx->state, parent, var);
  }
}

static
Debug_var*
debug_add_var(Debug_var_definition_context *ctx, Debug_var_type type, char *name) {
  
  Debug_var *var = debug_add_var(ctx->state, type, name);
  debug_add_var_to_default_group(ctx, var);
  
  return var;
}

static
Debug_var*
debug_add_root_group(Debug_state *debug_state, char *name) {
  Debug_var *group = debug_add_var(debug_state, Debug_var_type_var_group, name);
  
  debug_list_init(&group->var_group);
  
  return group;
}

static
Debug_var*
debug_begin_variable_group(Debug_var_definition_context *ctx, char *name) {
  Debug_var *group = debug_add_root_group(ctx->state, name);
  debug_add_var_to_default_group(ctx, group);
  
  assert(ctx->group_depth < array_count(ctx->group_stack) - 1);
  ctx->group_stack[++ctx->group_depth] = group;
  
  return group;
}

static 
Debug_var*
debug_add_var(Debug_var_definition_context *ctx, char *name, Bitmap_id value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_bitmap_display, name);
  var->bitmap_display.id = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_definition_context* ctx, char *name, bool value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_bool, name);
  var->bool_val = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_definition_context* ctx, char *name, i32 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_i32, name);
  var->int32 = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_definition_context* ctx, char *name, f32 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_f32, name);
  var->float32 = value;
  
  return var;
}

static
Debug_var*
debug_add_var(Debug_var_definition_context* ctx, char *name, v4 value) {
  Debug_var *var = debug_add_var(ctx, Debug_var_type_v4, name);
  var->vec4 = value;
  
  return var;
}

static
void
debug_end_variable_group(Debug_var_definition_context *ctx) {
  assert(ctx->group_depth > 0);
  ctx->group_depth--;
}

static
void
debug_create_vars(Debug_var_definition_context *ctx) {
  
#define debug_variable_listing(name) debug_add_var(ctx, #name, DEBUG_UI_##name);
  
  {
    debug_begin_variable_group(ctx, "entities");
    debug_variable_listing(draw_entity_outlines);
    debug_variable_listing(draw_all_entity_outlines);
    debug_end_variable_group(ctx);
  }
  
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
