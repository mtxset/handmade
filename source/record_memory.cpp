

// records game memory to memory and input to file

static 
void win32_check_record_index(i32 index) {
  Win32_state win_state {};
  assert(index < array_count(win_state.recording_memory));
  assert(index >= 0);
}

static 
void win32_begin_recording_input(Win32_state* win_state, i32 index) {
  win32_check_record_index(index);
  
  win_state->recording_input_index = index;
  char file_name[MAX_PATH];
  win32_get_input_file_location(win_state, index, sizeof(file_name), file_name);
  
  win_state->recording_file_handle = CreateFileA(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  
  win_state->recording_memory[index] = VirtualAlloc(0, win_state->total_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  assert(win_state->recording_memory[index]);
  CopyMemory(win_state->recording_memory[index], win_state->game_memory_block, win_state->total_memory_size);
}

static 
void win32_end_recording_input(Win32_state* win_state) {
  CloseHandle(win_state->recording_file_handle);
  win_state->recording_input_index = 0;
}

static 
void win32_begin_input_playback(Win32_state* win_state, i32 index) {
  win32_check_record_index(index);
  
  win_state->playing_input_index = index;
  char file_name[MAX_PATH];
  win32_get_input_file_location(win_state, index, sizeof(file_name), file_name);
  win_state->playing_file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  
  assert(win_state->recording_memory[index]);
  CopyMemory(win_state->game_memory_block, win_state->recording_memory[index], win_state->total_memory_size);
}

static 
void win32_end_input_playback(Win32_state* win_state) {
  CloseHandle(win_state->playing_file_handle);
  win_state->playing_input_index = 0;
}

static 
void win32_record_input(Win32_state* win_state, Game_input* new_input) {
  DWORD bytes_written;
  WriteFile(win_state->recording_file_handle, new_input, sizeof(*new_input), &bytes_written, 0);
}

static 
void win32_playback_input(Win32_state* win_state, Game_input* new_input) {
  DWORD bytes_read = 0;
  if (ReadFile(win_state->playing_file_handle, new_input, sizeof(*new_input), &bytes_read, 0)) {
    if (bytes_read == 0) {
      i32 playing_index = win_state->playing_input_index;
      
      win32_end_input_playback(win_state);
      win32_begin_input_playback(win_state, playing_index);
      
      // explanation: https://youtu.be/xrUSrVvB21c?t=4768
      ReadFile(win_state->playing_file_handle, new_input, sizeof(*new_input), &bytes_read, 0);
    }
  }
}
