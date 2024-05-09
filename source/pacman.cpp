void pacman_update(Game_bitmap_buffer* bitmap_buffer, Game_state* game_state, Game_input* input) {
  
  Pacman_state* pacman_state = &game_state->pacman_state;
  
  const i32 cols = 16*2;
  const i32 rows = 9*2;
  local_persist u32 tile_map[cols][rows] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 2, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1},
    {1, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 1},
    {1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 1},
    {1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 1},
    {1, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };
  
  f32 tile_width  = bitmap_buffer->window_width  / (f32)cols;
  f32 tile_height = bitmap_buffer->window_height / (f32)rows;
  
  // draw tiles
  {
    color_f32 color_tile  = { .0f, .5f, 1.0f};
    color_f32 color_empty = { .3f, .3f, 1.0f};
    color_f32 color_food  = { .9f, .0f, 1.0f };
    
    clear_screen(bitmap_buffer, color_gray_byte);
    
    for (i32 col = 0; col < rows; col++) {
      for (i32 row = 0; row < rows; row++) {
        i32 tile_id = tile_map[col][row];
        
        f32 start_x = (f32)row * tile_width;
        f32 start_y = (f32)col * tile_height;
        
        f32 end_x = start_x + tile_width;
        f32 end_y = start_y + tile_height;
        
        auto color = color_empty;
        
        if (tile_id == 1)
          color = color_tile;
        else if (tile_id == 2) {
          draw_rect(bitmap_buffer, start_x, start_y, end_x, end_y, color);
          f32 square_radius = 5.0f;
          start_x += tile_width / 2 - square_radius;
          start_y += tile_width / 2 - square_radius;
          end_x = start_x + square_radius * 2;
          end_y = start_y + square_radius * 2;
          color = color_food;
        }
        
        draw_rect(bitmap_buffer, start_x, start_y, end_x, end_y, color);
      }
    }
  }
  
  // move player
  {
    f32 player_x_delta = .0f;
    f32 player_y_delta = .0f;
    f32 move_offset = 64.0f;
    
    auto input_state = get_gamepad(input, 0);
    
    if (input_state->is_analog) {
      // TODO: fix so y in analog and digital is same (-1 or 1) it is consistent with keyboard and check dpad
      if (input_state->move_up.ended_down)    player_y_delta = 1.0f;
      if (input_state->move_down.ended_down)  player_y_delta = -1.0f;
      if (input_state->move_left.ended_down)  player_x_delta = -1.0f;
      if (input_state->move_right.ended_down) player_x_delta = 1.0f;
    } 
    else {
      // digital
      if (input_state->up.ended_down)    player_y_delta = -1.0f;
      if (input_state->down.ended_down)  player_y_delta = 1.0f;
      if (input_state->left.ended_down)  player_x_delta = -1.0f;
      if (input_state->right.ended_down) player_x_delta = 1.0f;
    }
    
    i32 new_player_tile_x = pacman_state->player_tile_x + (i32)player_x_delta;
    i32 new_player_tile_y = pacman_state->player_tile_y + (i32)player_y_delta;
    
    if (pacman_state->can_move)
    {
      i32 tile_id = tile_map[new_player_tile_y][new_player_tile_x];
      if (pacman_state->player_tile_x != new_player_tile_x && pacman_state->player_tile_y != new_player_tile_y) {
        new_player_tile_y = pacman_state->player_tile_y;
      }
      
      if (tile_id == 0) {
        if (pacman_state->player_tile_x != new_player_tile_x || pacman_state->player_tile_y != new_player_tile_y) {
          pacman_state->can_move = false;
          pacman_state->move_timer = 0;
        }
        
        pacman_state->player_tile_x = new_player_tile_x;
        pacman_state->player_tile_y = new_player_tile_y;
      }
      else if (tile_id == 2) {
        tile_map[new_player_tile_y][new_player_tile_x] = 0;
      }
    }
    
    pacman_state->move_timer += input->time_delta;
    if (pacman_state->move_timer >= .4f) {
      pacman_state->can_move = true;
    }
  }
  
  // move ghost
  {
    local_persist i32 random_number_index = 0;
    if ((pacman_state->ghost_move_timer += input->time_delta) >= 0.01f) {
      pacman_state->ghost_move_timer = 0;
      
      i32 new_tile_x = -1;
      i32 new_tile_y = -1;
      
      // try move in previous direction
      
      new_tile_x = pacman_state->ghost_tile_x + pacman_state->ghost_direction_x;
      new_tile_y = pacman_state->ghost_tile_y + pacman_state->ghost_direction_y;
      
      i32 circuit_breaker = 0;
      while (tile_map[new_tile_y][new_tile_x] != 0) {
        i32 x_offset = 0;
        i32 y_offset = 0;
        u32 random_choise = random_number_table[random_number_index++] % 2;
        
        if (random_choise == 0) {
          x_offset = 1;
          u32 random_dir = random_number_table[random_number_index++] % 2;
          if (random_dir == 0) {
            x_offset = -1;
          }
        }
        else {
          y_offset = 1;
          u32 random_dir = random_number_table[random_number_index++] % 2;
          if (random_dir == 0) {
            y_offset = -1;
          }
        }
        
        new_tile_x = pacman_state->ghost_tile_x + x_offset;
        new_tile_y = pacman_state->ghost_tile_y + y_offset;
        
        pacman_state->ghost_direction_x = x_offset;
        pacman_state->ghost_direction_y = y_offset;
        
        assert(circuit_breaker++ <= 500);
      }
      
      pacman_state->ghost_tile_x = new_tile_x;
      pacman_state->ghost_tile_y = new_tile_y;
    }
  }
  
  // draw ghost
  {
    color_f32 ghost_color = { .5f, .5f, .5f };
    f32 square_radius = 10.0f;
    
    f32 start_x = (f32)pacman_state->ghost_tile_x * tile_width;
    f32 start_y = (f32)pacman_state->ghost_tile_y * tile_height;
    
    start_x += tile_width / 2 - square_radius;
    start_y += tile_height / 2 - square_radius;
    
    f32 end_x = start_x + square_radius * 2;
    f32 end_y = start_y + square_radius * 2;
    
    draw_rect(bitmap_buffer, start_x, start_y, end_x, end_y, ghost_color);
  }
  
  // draw player
  {
    f32 square_radius = 10.0f;
    
    f32 start_x = (f32)pacman_state->player_tile_x * tile_width;
    f32 start_y = (f32)pacman_state->player_tile_y * tile_height;
    
    start_x += tile_width / 2 - square_radius;
    start_y += tile_height / 2 - square_radius;
    
    f32 end_x = start_x + square_radius * 2;
    f32 end_y = start_y + square_radius * 2;
    
    draw_rect(bitmap_buffer, start_x, start_y, end_x, end_y, FRGB_GOLD);
  }
  
}