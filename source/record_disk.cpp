#include "main.h"

// records input and game memory to a file
// I'm skipping part of implementing loading from file https://www.youtube.com/watch?v=es-Bou2dIdY

static void
win32_begin_recording_input(win32_state* win_state, int index) {
    win_state->recording_input_index = index;
    
    char file_name[MAX_PATH];
    win32_get_input_file_location(win_state, index, sizeof(file_name), file_name);
    
    win_state->recording_file_handle = CreateFileA(file_name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    
    DWORD bytes_to_write = (DWORD)win_state->total_memory_size;
    macro_assert(win_state->total_memory_size == bytes_to_write);
    DWORD bytes_written;
    WriteFile(win_state->recording_file_handle, win_state->game_memory_block, bytes_to_write, &bytes_written, 0);
}

static void
win32_end_recording_input(win32_state* win_state) {
    CloseHandle(win_state->recording_file_handle);
    win_state->recording_input_index = 0;
}

static void
win32_begin_input_playback(win32_state* win_state, int index) {
    win_state->playing_input_index = index;
    
    char file_name[MAX_PATH];
    win32_get_input_file_location(win_state, index, sizeof(file_name), file_name);
    
    win_state->playing_file_handle = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    
    DWORD bytes_to_read = (DWORD)win_state->total_memory_size;
    macro_assert(win_state->total_memory_size == bytes_to_read);
    DWORD bytes_read;
    ReadFile(win_state->playing_file_handle, win_state->game_memory_block, bytes_to_read, &bytes_read, 0);
}

static void
win32_end_input_playback(win32_state* win_state) {
    CloseHandle(win_state->playing_file_handle);
    win_state->playing_input_index = 0;
}

static void
win32_record_input(win32_state* win_state, game_input* new_input) {
    DWORD bytes_written;
    WriteFile(win_state->recording_file_handle, new_input, sizeof(*new_input), &bytes_written, 0);
}

static void
win32_playback_input(win32_state* win_state, game_input* new_input) {
    DWORD bytes_read = 0;
    if (ReadFile(win_state->playing_file_handle, new_input, sizeof(*new_input), &bytes_read, 0)) {
        if (bytes_read == 0) {
            int playing_index = win_state->playing_input_index;
            
            win32_end_input_playback(win_state);
            win32_begin_input_playback(win_state, playing_index);
            
            // explanation: https://youtu.be/xrUSrVvB21c?t=4768
            ReadFile(win_state->playing_file_handle, new_input, sizeof(*new_input), &bytes_read, 0);
        }
    }
}
