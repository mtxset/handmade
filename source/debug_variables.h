/* date = October 30th 2025 6:27 pm */

#ifndef DEBUG_VARIABLES_H
#define DEBUG_VARIABLES_H

#define debug_variable_listing(name) Debug_var_type_bool, #name, name

Debug_var debug_var_list[] = {
  debug_variable_listing(DEBUG_UI_use_debug_camera),
  debug_variable_listing(DEBUG_UI_ground_chunks_outlines),
  debug_variable_listing(DEBUG_UI_particle_test),
  debug_variable_listing(DEBUG_UI_particle_grid),
  debug_variable_listing(DEBUG_UI_use_space_outlines),
  debug_variable_listing(DEBUG_UI_ground_chunk_checker_board),
  debug_variable_listing(DEBUG_UI_redo_ground_chunks_on_recompile),
  
  debug_variable_listing(DEBUG_UI_test_weird_draw_buffer_size),
  debug_variable_listing(DEBUG_UI_familiar_follows_hero),
  debug_variable_listing(DEBUG_UI_show_lighting_samples),
  debug_variable_listing(DEBUG_UI_use_room_based_camera),
};

#endif //DEBUG_VARIABLES_H
