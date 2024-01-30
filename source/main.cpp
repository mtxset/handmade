// https://youtu.be/8L21Tyh53BQ?t=3915
// there is some bug which was introduced on day 78 with bottom stairs not having collision

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include "main.h"
#include "utils.h"
#include "utils.cpp"
#include "game.h" 

// some race condition happening and introduced in 126 day
// does not crash on -Od, crashes with -O2
// most likely somehere race condition happens
// very lame
// running as exe as admin helps???
#pragma optimize("", off)

/* Add to win32 layer
- save games locations
- getting handle of our own executable
- asset loading path
- raw input
- threading
- sleep/timeBeginPeriod
- ClipCursor()
- fullscreen support
- WM_SETCURSOR (cursor visibility)
- QueryCancelAutoplay 
- Hardware acceleration opengl or dx
...
*/

// if for recording of input and gamestate we use disk this will allocate file of PERMANENT_MEMORY_SIZE_MB + TRANSIENT_MEMORY_SIZE_MB size, and that will hang game till it's written
global_var const i32           PERMANENT_MEMORY_SIZE_MB = 64;
global_var const i32           TRANSIENT_MEMORY_SIZE_MB = 256;

global_var bool                Global_game_running = true;
global_var bool                Global_pause_sound_debug_sync = false;

global_var Win32_game_code     Global_game_code;
global_var Win32_bitmap_buffer Global_backbuffer;
global_var HBITMAP             Global_bitmap_handle;
global_var HDC                 Global_bitmap_device_context;
global_var LPDIRECTSOUNDBUFFER Global_sound_buffer;
global_var i64                 Global_perf_freq;
global_var bool                Global_show_cursor;
global_var WINDOWPLACEMENT     Global_window_last_position = { sizeof(Global_window_last_position) };
global_var bool                Global_fullscreen = false;

// making sure that if we don't have links to functions we don't crash because we use stubs
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);            // test 
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);    // test
global_var x_input_get_state* XInputGetState_;
global_var x_input_set_state* XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

typedef HRESULT WINAPI direct_sound_create(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter); // define delegate
global_var direct_sound_create* DirectSoundCreate_;                                                             // define variable to hold it
#define DirectSoundCreate DirectSoundCreate_                                                                // change name by which we reference upper-line mentioned variable

internal
void
win32_get_exe_filename(Win32_state* win_state) {
  auto current_file_name_size = GetModuleFileNameA(0, win_state->exe_file_name, sizeof(win_state->exe_file_name));
  win_state->last_slash = win_state->exe_file_name;
  for (char* scan = win_state->exe_file_name; *scan; scan++) {
    if (*scan == '\\')
      win_state->last_slash = scan + 1;
  }
}

internal
void
win32_build_exe_filename(Win32_state* win_state, char* filename, i32 dest_count, char* dest) {
  string_concat(win_state->last_slash - win_state->exe_file_name, win_state->exe_file_name, 
                string_len(filename), filename, dest_count, dest);
}

internal
void
win32_get_input_file_location(Win32_state* win_state, i32 index, i32 dest_count, char* dest) {
  char* file_name = "game.input";
  win32_build_exe_filename(win_state, file_name, dest_count, dest); 
}

// disgusting
#include "record_memory.cpp"

internal
void
win32_init_direct_sound(HWND window, i32 samples_per_second, i32 buffer_size) {
  // NOTE: Load the library
  HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
  
  if (DSoundLibrary) {
    // NOTE: Get a DirectSound Object
    direct_sound_create* DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    
    // TODO: Double-check that this works on XP - DirectSound or 7??
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      WAVEFORMATEX wave_format = {};
      wave_format.wFormatTag = WAVE_FORMAT_PCM;
      wave_format.nChannels = 2;
      wave_format.nSamplesPerSec = samples_per_second;
      wave_format.wBitsPerSample = 16;
      wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
      wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
      wave_format.cbSize = 0;
      
      if (SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
        DSBUFFERDESC buffer_desc = {};
        buffer_desc.dwSize = sizeof(buffer_desc);
        buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        
        // NOTE: "Create" a primary buffer
        // TODO: DSBCAPS_GLOBALFOCUS?
        LPDIRECTSOUNDBUFFER primary_buffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&buffer_desc, &primary_buffer, 0))) {
          HRESULT Error = primary_buffer->SetFormat(&wave_format);
          if (SUCCEEDED(Error)) {
            // NOTE: We have finnaly set the format!
            // printf("Primary buffer format was set.\n");
          } else {
            // TODO: Diagnostic
          }
        } else {
          // TODO: Diagnostic
        }
      } else {
        // TODO: Diagnostic
      }
      
      // NOTE: "Create" a secondary buffer
      // TODO: DSBCAPS_GETCURRENTPOSITION2
      DSBUFFERDESC buffer_desc = {};
      buffer_desc.dwSize = sizeof(buffer_desc);
      buffer_desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
      buffer_desc.dwBufferBytes = buffer_size;
      buffer_desc.lpwfxFormat = &wave_format;
      HRESULT Error = DirectSound->CreateSoundBuffer(&buffer_desc, &Global_sound_buffer, 0);
      if (SUCCEEDED(Error)) {
        // printf("Secondary buffer created successfully.\n");
      } else {
      }
      
      // NOTE: Start it playing
    } else {
      // TODO: Diagnostic
    }
  }
}

internal
void
win32_clear_sound_buffer(Win32_sound_output* sound_output) {
  void* region_one;
  DWORD region_one_size;
  void* region_two;
  DWORD region_two_size;
  
  if (!SUCCEEDED(Global_sound_buffer->Lock(0, sound_output->buffer_size, 
                                           &region_one, &region_one_size, 
                                           &region_two, &region_two_size, 0))) {
    // too much fails OutputDebugStringA("Failed to lock global sound buffer\n");
    return;
  }
  
  auto sample_out = (i8*)region_one;
  for (DWORD index = 0; index < region_one_size; index++) {
    *sample_out++ = 0;
  }
  
  sample_out = (i8*)region_two;
  for (DWORD index = 0; index < region_two_size; index++) {
    *sample_out++ = 0;
  }
  
  Global_sound_buffer->Unlock(region_one, region_one_size, region_two, region_two_size);
}

internal
void
win32_fill_sound_buffer(Win32_sound_output* sound_output, DWORD bytes_to_lock, DWORD bytes_to_write, Game_sound_buffer* source_buffer) {
  void* region_one;
  DWORD region_one_size;
  void* region_two;
  DWORD region_two_size;
  
  if (!SUCCEEDED(Global_sound_buffer->Lock(bytes_to_lock, bytes_to_write, 
                                           &region_one, &region_one_size, 
                                           &region_two, &region_two_size, 0))) {
    // too much fails OutputDebugStringA("Failed to lock global sound buffer\n");
    return;
  }
  
  auto sample_out = (i16*)region_one;
  auto source_out = (i16*)source_buffer->samples;
  DWORD region_sample_count = region_one_size / sound_output->bytes_per_sample;
  
  for (DWORD sample_index = 0; sample_index < region_sample_count; sample_index++) {
    *sample_out++ = *source_out++;
    *sample_out++ = *source_out++;
    
    sound_output->running_sample_index++;
  }
  
  sample_out = (i16*)region_two;
  DWORD region_sample_count_two = region_two_size / sound_output->bytes_per_sample;
  
  for (DWORD sample_index = 0; sample_index < region_sample_count_two; sample_index++) {
    *sample_out++ = *source_out++;
    *sample_out++ = *source_out++;
    
    sound_output->running_sample_index++;
  }
  
  Global_sound_buffer->Unlock(region_one, region_one_size, region_two, region_two_size);
}

internal
f32
win32_xinput_cutoff_deadzone(SHORT thumb_value) {
  f32 result = 0;
  auto dead_zone_threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
  if        (thumb_value > dead_zone_threshold) {
    result = (f32)(thumb_value + dead_zone_threshold) / (32767 - dead_zone_threshold);
  } else if (thumb_value < -dead_zone_threshold) {
    result = (f32)(thumb_value + dead_zone_threshold) / (32768 - dead_zone_threshold);
  }
  
  return result;
}

internal
FILETIME
win32_get_last_write_time(char* filename) {
  FILETIME result = {};
  
  WIN32_FILE_ATTRIBUTE_DATA file_data;
  if (GetFileAttributesEx(filename, GetFileExInfoStandard, &file_data)) {
    result = file_data.ftLastWriteTime;
  }
  
  return result;
}

internal
Win32_game_code
win32_load_game_code(char* source_dll_filepath, char* source_temp_filepath, char* lock_filepath) {
  Win32_game_code result = {};
  
  WIN32_FILE_ATTRIBUTE_DATA _;
  if (GetFileAttributesEx(lock_filepath, GetFileExInfoStandard, &_)) {
    goto exit;
  }
  
  result.dll_last_write_time = win32_get_last_write_time(source_dll_filepath);
  
  CopyFile(source_dll_filepath, source_temp_filepath, FALSE);
  result.game_code_dll = LoadLibraryA(source_temp_filepath);
  
  if (!result.game_code_dll)
    goto exit;
  
  result.update_and_render = (game_update_render_signature*)GetProcAddress(result.game_code_dll, "game_update_render");
  result.get_sound_samples = (game_get_sound_samples_signature*)GetProcAddress(result.game_code_dll, "game_get_sound_samples");
  
  result.valid = (result.update_and_render && result.get_sound_samples);
  
  exit:
  if (!result.valid) {
    result.update_and_render = 0;
    result.get_sound_samples = 0;
  }
  
  return result;
}

internal
void
win32_unload_game_code(Win32_game_code* game_code) {
  if (game_code->game_code_dll) {
    FreeLibrary(game_code->game_code_dll);
    game_code->game_code_dll = 0;
  }
  
  game_code->valid = false;
  game_code->update_and_render = 0;
  game_code->get_sound_samples = 0;
}

internal
bool
win32_load_xinput() {
  // looks locally, looks in windows
  // support only for some windows
  auto xinput_lib = LoadLibraryA("xinput1_4.dll");
  
  if (!xinput_lib)
    xinput_lib = LoadLibraryA("xinput1_3.dll");
  
  if (!xinput_lib)
    xinput_lib = LoadLibraryA("xinput9_1_0.dll");
  
  if (xinput_lib) {
    XInputGetState = (x_input_get_state*)GetProcAddress(xinput_lib, "XInputGetState");
    XInputSetState = (x_input_set_state*)GetProcAddress(xinput_lib, "XInputSetState");
    
    if (!XInputGetState || !XInputSetState)
      return false;
  } else {
    return false;
  }
  
  return true;
}

internal
Win32_window_dimensions
get_window_dimensions(HWND window) {
  
  Win32_window_dimensions result;
  
  RECT clientRect;
  GetClientRect(window, &clientRect);
  
  result.height = clientRect.bottom - clientRect.top;
  result.width = clientRect.right - clientRect.left;
  
  return result;
}

// DIB - device independant section
internal
void
win32_resize_dib_section(Win32_bitmap_buffer* bitmap_buffer, i32 width, i32 height) {
  
  if (bitmap_buffer->memory) {
    VirtualFree(bitmap_buffer->memory, 0, MEM_RELEASE);
  }
  
  bitmap_buffer->width = width;
  bitmap_buffer->height = height;
  bitmap_buffer->bytes_per_pixel = 4;
  bitmap_buffer->pitch = align_16(bitmap_buffer->bytes_per_pixel * width);
  
  bitmap_buffer->info.bmiHeader.biSize = sizeof(bitmap_buffer->info.bmiHeader);
  bitmap_buffer->info.bmiHeader.biWidth = width;
  bitmap_buffer->info.bmiHeader.biHeight = height; // draw left-to-right, bottom-to-top
  bitmap_buffer->info.bmiHeader.biPlanes = 1;
  bitmap_buffer->info.bmiHeader.biBitCount = 32;
  bitmap_buffer->info.bmiHeader.biCompression = BI_RGB;
  
  // we are taking 4 bytes (8 bits for each color (rgb) + aligment 8 bits = 32 bits) for each pixel
  auto bitmap_memory_size = bitmap_buffer->pitch * height;
  // virtual alloc allocates region of page which, one page size is 1 mb? GetSystemInfo can output that info
  bitmap_buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal
void
win32_display_buffer_to_window(Win32_bitmap_buffer* bitmap_buffer, HDC device_context, i32 window_width, i32 window_height) {
  i32 offset_x = 10;
  i32 offset_y = 10;
  
  if (!Global_fullscreen) {
    PatBlt(device_context, 0, 0, window_width, offset_x, BLACKNESS);
    PatBlt(device_context, 0, offset_x, offset_y, bitmap_buffer->height, BLACKNESS);
    PatBlt(device_context, 0, offset_x + bitmap_buffer->height, window_width, window_height - bitmap_buffer->height - offset_x, BLACKNESS);
    PatBlt(device_context, offset_y + bitmap_buffer->width, offset_x, window_width - offset_y - bitmap_buffer->width, bitmap_buffer->height, BLACKNESS);
    
    window_width  = bitmap_buffer->width;
    window_height = bitmap_buffer->height;
  }
  else {
    // don't stretch just increase proportianally
    bool stretch = false;
    
    if (!stretch) {
      // center out
      offset_y = 0;
      offset_x = window_width / 2 - bitmap_buffer->width;
      window_width  = bitmap_buffer->width * 2;
      window_height = bitmap_buffer->height * 2;
      
      i32 end_of_bitmap_x = window_width + offset_x;
      
      PatBlt(device_context, 0, 0, offset_x, window_height, BLACKNESS);
      PatBlt(device_context, end_of_bitmap_x, 0, offset_x, end_of_bitmap_x, BLACKNESS);
    }
  }
  
  StretchDIBits(device_context, 
                offset_x, offset_y, window_width, window_height, 
                0, 0, bitmap_buffer->width, bitmap_buffer->height, 
                bitmap_buffer->memory, &bitmap_buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal
void
win32_process_xinput_button(DWORD xinput_button_state, DWORD button_bit, Game_button_state* old_state, Game_button_state* new_state) {
  new_state->ended_down = (xinput_button_state & button_bit) == button_bit;
  new_state->half_transition_count = old_state->ended_down != new_state->ended_down ? 1 : 0;
}

internal
void
win32_process_keyboard_input(Game_button_state* new_state, bool is_down) {
  if (new_state->ended_down != is_down){ 
    new_state->ended_down = is_down;
    new_state->half_transition_count++;
  }
}

/*
    * Raymond Chen: https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    * this will just set window to take "fullscreen" but it won't be able to affect refresh rate
* for that use: ChangeDisplaySettings
*/
internal
void
toggle_fullscreen(HWND window_handle) {
  DWORD style = GetWindowLong(window_handle, GWL_STYLE);
  
  if (style & WS_OVERLAPPEDWINDOW) {
    MONITORINFO monitor_info = { sizeof(monitor_info) };
    if (GetWindowPlacement(window_handle, &Global_window_last_position) && GetMonitorInfo(MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
      
      SetWindowLong(window_handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
      
      SetWindowPos(window_handle, HWND_TOP,
                   monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                   monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                   monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      
      Global_fullscreen = true;
      Global_show_cursor = false;
    }
  } 
  else {
    SetWindowLong(window_handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(window_handle, &Global_window_last_position);
    SetWindowPos(window_handle, NULL, 
                 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    
    Global_fullscreen = false;
    Global_show_cursor = true;
  }
}

internal
void 
win32_handle_messages(Win32_state* win_state, Game_controller_input* keyboard_input) {
  MSG message;
  while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
    switch (message.message) {
      case WM_QUIT: {
        Global_game_running = false;
      } break;
      case WM_KEYUP:
      case WM_KEYDOWN:
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN: {
        auto vk_key = message.wParam;
        bool previous_state = (message.lParam & (1 << 30)) != 0; // will return 0 or bit 30
        bool was_down       = (message.lParam & (1 << 30)) != 0; // if I get 0 I get true if I get something besides zero I compare it to zero and I will get false
        bool is_down        = (message.lParam & (1 << 31)) == 0; // parenthesis required because == has precedence over &
        bool alt_is_down    = (message.lParam & (1 << 29)) != 0; // will return 0 or bit 29; if I get 29 alt is down - if 0 it's not so I compare it to 0
        
        if      (vk_key == 'W') {
          win32_process_keyboard_input(&keyboard_input->up, is_down);
        } 
        else if (vk_key == 'S') {
          win32_process_keyboard_input(&keyboard_input->down, is_down);
        } 
        else if (vk_key == 'A') {
          win32_process_keyboard_input(&keyboard_input->left, is_down);
        } 
        else if (vk_key == 'D') {
          win32_process_keyboard_input(&keyboard_input->right, is_down);
        } 
        else if (vk_key == VK_UP) {
          win32_process_keyboard_input(&keyboard_input->action_up, is_down);
        } 
        else if (vk_key == VK_LEFT) {
          win32_process_keyboard_input(&keyboard_input->action_left, is_down);
        } 
        else if (vk_key == VK_DOWN) {
          win32_process_keyboard_input(&keyboard_input->action_down, is_down);
        } 
        else if (vk_key == VK_RIGHT) {
          win32_process_keyboard_input(&keyboard_input->action_right, is_down);
        }
        else if (vk_key == VK_SPACE) {
          win32_process_keyboard_input(&keyboard_input->action, is_down);
        } 
        else if (vk_key == VK_ESCAPE) {
          win32_process_keyboard_input(&keyboard_input->back, is_down);
        } 
        else if (vk_key == VK_RETURN) {
          win32_process_keyboard_input(&keyboard_input->start, is_down);
        } 
        else if (vk_key == VK_F4 && alt_is_down || vk_key == VK_F6) {
          Global_game_running = false;
        }
        else if (vk_key == VK_SHIFT) {
          win32_process_keyboard_input(&keyboard_input->shift, is_down);
        }
        else if (vk_key == 'P' && is_down) {
          Global_pause_sound_debug_sync = !Global_pause_sound_debug_sync;
        }
        else if (vk_key == VK_F2 && is_down) {
          if (win_state->playing_input_index == 0) {
            if (win_state->recording_input_index == 0) {
              win32_begin_recording_input(win_state, 1);
            } 
            else {
              win32_end_recording_input(win_state);
              win32_begin_input_playback(win_state, 1);
            }
          } 
          else {
            win32_end_input_playback(win_state);
          }
          
        }
        
        if (vk_key == VK_RETURN && alt_is_down && is_down) { // alt enter
          if (message.hwnd)
            toggle_fullscreen(message.hwnd);
        }
        
      } break;
      default: {
        TranslateMessage(&message);
        DispatchMessage(&message);
      } break;
    }
  }
}

LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
    case WM_SETCURSOR: {
      if (Global_show_cursor)
        result = DefWindowProcA(window, message, wParam, lParam);
      else // hide cursor
        SetCursor(0);
    } break;
    case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN: {
      macro_assert(!"Should never hit it here");
    } break;
    case WM_SIZE: {
    } break;
    
    case WM_DESTROY: {
      Global_game_running = false;
    } break;
    
    case WM_CLOSE: {
      Global_game_running = false;
    } break;
    
    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;
    
    case WM_PAINT: {
      PAINTSTRUCT paint;
      auto device_context = BeginPaint(window, &paint);
      
      auto x = paint.rcPaint.left;
      auto y = paint.rcPaint.top;
      auto width = paint.rcPaint.right - paint.rcPaint.left;
      auto height = paint.rcPaint.bottom - paint.rcPaint.top;
      
      auto dimensions = get_window_dimensions(window);
      win32_display_buffer_to_window(&Global_backbuffer, device_context, dimensions.width, dimensions.height);
      EndPaint(window, &paint);
    } break;
    
    default: {
      result = DefWindowProcA(window, message, wParam, lParam);
    } break;
  }
  
  return result;
}

LARGE_INTEGER 
win32_get_wall_clock() {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

f32 
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
  return (f32)(end.QuadPart - start.QuadPart) / Global_perf_freq;
}

internal 
void 
win32_debug_draw_vertical_line(Win32_bitmap_buffer* backbuffer, i32 x, i32 top, i32 bottom, u32 color) {
  
  if (x < 0 || x >= backbuffer->width)
    return;
  
  if (top <= 0)
    top = 0;
  
  if (bottom > backbuffer->height)
    bottom = backbuffer->height;
  
  auto pixel = (u8*)backbuffer->memory + x * backbuffer->bytes_per_pixel + top * backbuffer->pitch;
  for (i32 i = top; i < bottom; i++) {
    *(u32*)pixel = color;
    pixel += backbuffer->pitch;
  }
}

void 
win32_draw_sound_buffer_marker(Win32_bitmap_buffer* backbuffer, Win32_sound_output* sound_output, f32 ratio, i32 pad_x, i32 top, i32 bottom, DWORD value, u32 color) {
  auto float_x = ratio * (f32)value;
  auto x = pad_x + (i32)float_x;
  
  win32_debug_draw_vertical_line(backbuffer, x, top, bottom, color);
}

void 
win32_debug_sync_display(Win32_bitmap_buffer* backbuffer, i32 marker_count, i32 current_marker_index, Win32_debug_time_marker* markers, Win32_sound_output* sound_output, f32 target_seconds_per_frame) {
  i32 x_padding = 16, y_padding = 16;
  
  i32 line_height = 64;
  
  f32 sound_to_video_width_ratio = (f32)(backbuffer->width - 2 * x_padding) / (f32)sound_output->buffer_size; 
  f32 r = sound_to_video_width_ratio;
  
  for (i32 i = 0; i < marker_count; i++) {
    DWORD play_color           = 0xffffffff;
    DWORD write_color          = 0xffff0000;
    DWORD expected_write_color = 0xffffff00;
    DWORD play_window_color    = 0xffff00ff;
    
    i32 top = y_padding;
    i32 bottom = y_padding + line_height;
    
    auto current_marker = &markers[i];
    macro_assert(current_marker->output_play_cursor  < sound_output->buffer_size);
    macro_assert(current_marker->play_cursor         < sound_output->buffer_size);
    macro_assert(current_marker->output_write_cursor < sound_output->buffer_size);
    macro_assert(current_marker->write_cursor        < sound_output->buffer_size);
    macro_assert(current_marker->output_location     < sound_output->buffer_size);
    macro_assert(current_marker->output_bytes        < sound_output->buffer_size);
    
    if (i == current_marker_index) {
      top += line_height + y_padding;
      bottom += line_height + y_padding;
      
      auto first_top = top;
      
      win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                     top, bottom, 
                                     current_marker->output_play_cursor, play_color);
      
      win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                     top, bottom, 
                                     current_marker->output_write_cursor, write_color);
      
      top += line_height + y_padding;
      bottom += line_height + y_padding;
      
      win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                     top, bottom, 
                                     current_marker->output_location, play_color);
      win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                     top, bottom, 
                                     current_marker->output_location + current_marker->output_bytes, write_color);
      
      top += line_height + y_padding;
      bottom += line_height + y_padding;
      
      win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                     first_top, bottom, 
                                     current_marker->expected_play_cursor, expected_write_color);
    }
    
    win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                   top, bottom, 
                                   current_marker->play_cursor, play_color);
    
    win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                   top, bottom, 
                                   current_marker->play_cursor + 480*sound_output->bytes_per_sample, play_window_color);
    
    win32_draw_sound_buffer_marker(backbuffer, sound_output, r, x_padding, 
                                   top, bottom, 
                                   current_marker->write_cursor, write_color);
  }
}

internal
HWND
create_default_window(LRESULT win32_window_processor, HINSTANCE current_instance, char* class_name, i32 initial_window_width, i32 initial_window_height) {
  
  HWND result = {};
  
  WNDCLASSA window_class = {};
  
  window_class.style         = CS_HREDRAW | CS_HREDRAW; // redraw full window (vertical/horizontal) (streches)
  window_class.lpfnWndProc   = (WNDPROC)win32_window_processor;
  window_class.hInstance     = current_instance;
  window_class.lpszClassName = class_name;
  window_class.hCursor = LoadCursor(0, IDC_ARROW); // 0, otherwise it will try to load cursor from our executable
  
  if (RegisterClass(&window_class) == 0) {
    OutputDebugStringA("RegisterClass failed\n");
    return 0;
  }
  
  result = CreateWindowEx(0, window_class.lpszClassName, "GG", 
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          initial_window_width + 150,
                          initial_window_height + 150, 
                          0, 0, current_instance, 0);
  
  if (result == 0) {
    OutputDebugStringA("CreateWindow failed\n");
    return 0;
  }
  
  return result;
}

internal
void
handle_debug_cycle_count(Game_memory* memory) {
#if INTERNAL
  
  for (u32 counter_index = 0; counter_index < macro_array_count(memory->counter_list); counter_index++) {
    
    Debug_cycle_counter* counter = memory->counter_list + counter_index;
    
    if (counter->hit_count == 0) 
      continue;
    
    bool print_cycles = false;
    
    if (print_cycles) {
      OutputDebugStringA("Cycles\n");
      char buffer[256];
      u64 cycles_per_hit = counter->cycle_count / counter->hit_count;
      _snprintf_s(buffer, sizeof(buffer),
                  "%d: %I64u cy %uh %I64u cy/h\n", 
                  counter_index,
                  counter->cycle_count, counter->hit_count, cycles_per_hit);
      OutputDebugStringA(buffer);
    }
    
    counter->cycle_count = 0;
    counter->hit_count = 0;
  }
#endif
}

struct Platform_work_queue_entry {
  Platform_work_queue_callback *callback;
  void *data;
};

struct Platform_work_queue {
  u32 volatile completion_goal;
  u32 volatile completion_count;
  
  u32 volatile next_entry_to_write;
  u32 volatile next_entry_to_read;
  
  HANDLE semaphore;
  
  Platform_work_queue_entry entry_list[256];
};

struct Win32_thread_info {
  i32 index;
  Platform_work_queue *queue;
};

struct String_entry {
  char *str;
};

internal
void
win32_add_entry(Platform_work_queue *queue, Platform_work_queue_callback *callback, void *data) {
  u32 new_entry_to_write = (queue->next_entry_to_write + 1) % macro_array_count(queue->entry_list);
  
  macro_assert(new_entry_to_write != queue->next_entry_to_read);
  
  Platform_work_queue_entry *entry = queue->entry_list + queue->next_entry_to_write;
  
  entry->callback = callback;
  entry->data = data;
  
  ++queue->completion_goal;
  
  _WriteBarrier(); // tell compiler not move stores around
  // _mm_sfence(); not neccessary (cpu not to move around???)
  
  queue->next_entry_to_write = new_entry_to_write;
  
  // wake up thread
  ReleaseSemaphore(queue->semaphore, 1, 0);
}

internal
bool
win32_do_next_work_q_entry(Platform_work_queue *queue) {
  bool we_should_sleep = false;
  
  u32 original_next_entry_to_read = queue->next_entry_to_read;
  u32 new_entry_to_read = (original_next_entry_to_read + 1) % macro_array_count(queue->entry_list);
  
  if (original_next_entry_to_read != queue->next_entry_to_write) {
    
    u32 index = InterlockedCompareExchange((LONG volatile*)&queue->next_entry_to_read,
                                           new_entry_to_read,
                                           original_next_entry_to_read);
    
    if (index == original_next_entry_to_read) {
      Platform_work_queue_entry entry = queue->entry_list[index];
      entry.callback(queue, entry.data);
      InterlockedIncrement((LONG volatile*)&queue->completion_count);
    }
    
  }
  else {
    we_should_sleep = true;
  }
  
  return we_should_sleep;
}

internal
void
win32_complete_all_work(Platform_work_queue* queue) {
  while (queue->completion_goal != queue->completion_count) {
    win32_do_next_work_q_entry(queue);
  }
  
  queue->completion_count = 0;
  queue->completion_goal = 0;
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(do_worker_work) {
  
  char buffer[256];
  _snprintf_s(buffer, sizeof(buffer), "thread: %u; %s\n", GetCurrentThreadId(), (char*)data);
  OutputDebugStringA(buffer);
}

DWORD
WINAPI
thread_proc(LPVOID param) {
  Win32_thread_info *thread_info = (Win32_thread_info*)param;
  
  while (true) {
    if (win32_do_next_work_q_entry(thread_info->queue)) {
      WaitForSingleObjectEx(thread_info->queue->semaphore, INFINITE, 0);
    }
  }
}

i32
main(HINSTANCE current_instance, HINSTANCE previousInstance, LPSTR commandLineParams, i32 nothing) {
  
  Platform_work_queue queue = {};
  
  // create semaphore and threads
  {
    u32 initial_count = 0;
    Win32_thread_info info_list[7];
    
    u32 thread_count = macro_array_count(info_list);
    queue.semaphore = CreateSemaphoreEx(0,            // default security attributes
                                        initial_count,// initial count
                                        thread_count, // maximum count
                                        0,            // unnamed semaphore
                                        0,            // flags
                                        SEMAPHORE_ALL_ACCESS);
    
    macro_assert(queue.semaphore != NULL);
    
    for (u32 thread_index = 0; thread_index < thread_count; ++thread_index) {
      Win32_thread_info *info = info_list + thread_index;
      
      info->queue = &queue;
      info->index = thread_index;
      
      DWORD thread_id;
      HANDLE thread_handle = CreateThread(0, // security attributes
                                          0, // stack size  will default to the we have in current context
                                          thread_proc, // thread function
                                          info, // thread args
                                          0,    // start right away
                                          &thread_id);
      
      macro_assert(thread_handle != NULL);
      CloseHandle(thread_handle); // it will not terminate thread
      // but later in c runtime lib windows will call ExitProcess which will kill all threads
    }
  }
  
  // do some work with threads
  {
    win32_add_entry(&queue, do_worker_work, "msg a1");
    win32_add_entry(&queue, do_worker_work, "msg a2");
    win32_add_entry(&queue, do_worker_work, "msg a3");
    win32_add_entry(&queue, do_worker_work, "msg a4");
    win32_add_entry(&queue, do_worker_work, "msg a5");
    win32_add_entry(&queue, do_worker_work, "msg a6");
    win32_add_entry(&queue, do_worker_work, "msg a7");
    win32_add_entry(&queue, do_worker_work, "msg a8");
    win32_add_entry(&queue, do_worker_work, "msg a9");
    
    win32_add_entry(&queue, do_worker_work, "msg b1");
    win32_add_entry(&queue, do_worker_work, "msg b2");
    win32_add_entry(&queue, do_worker_work, "msg b3");
    win32_add_entry(&queue, do_worker_work, "msg b4");
    win32_add_entry(&queue, do_worker_work, "msg b5");
    win32_add_entry(&queue, do_worker_work, "msg b6");
    win32_add_entry(&queue, do_worker_work, "msg b7");
    win32_add_entry(&queue, do_worker_work, "msg b8");
    win32_add_entry(&queue, do_worker_work, "msg b9");
    
    win32_complete_all_work(&queue);
  }
  
  LARGE_INTEGER performance_freq, end_counter, last_counter, flip_wall_clock;
  QueryPerformanceFrequency(&performance_freq);
  Global_perf_freq = performance_freq.QuadPart;
  
  // load game code
  Win32_state win_state = {};
  Win32_game_code game_code = {};
  char game_code_dll_name[]      = "game.dll";
  char temp_game_code_dll_name[] = "game_temp.dll";
  char lock_file_name[]          = "lock.tmp";
  char game_code_full_path[MAX_PATH];
  char temp_game_code_full_path[MAX_PATH];
  char lock_file_full_path[MAX_PATH];
  {
    win32_get_exe_filename(&win_state);
    
    win32_build_exe_filename(&win_state, game_code_dll_name, sizeof(game_code_full_path), game_code_full_path);
    win32_build_exe_filename(&win_state, temp_game_code_dll_name, sizeof(temp_game_code_full_path), temp_game_code_full_path);
    win32_build_exe_filename(&win_state, lock_file_name, sizeof(lock_file_full_path), lock_file_full_path);
    
    game_code = win32_load_game_code(game_code_full_path, temp_game_code_full_path, lock_file_full_path);
  }
  
  UINT desired_scheduler_period_ms = 1;
  auto sleep_is_granular = timeBeginPeriod(desired_scheduler_period_ms) == TIMERR_NOERROR;
  
  // window class
  // setting resolution my screen is 16:10
  i32 initial_window_width  = 960;
  i32 initial_window_height = 540;
#if AR1610
  initial_window_width  = 960; // 2560
  initial_window_height = 600; // 1080 
#endif
  
#if 0
  initial_window_width  = 1920; // 2560
  initial_window_height = 1080; // 1080 
#endif
  
#if 0
  // win does not support pitch
  initial_window_width  = 1279;
  initial_window_height = 719;
#endif
  
  win32_resize_dib_section(&Global_backbuffer, initial_window_width, initial_window_height);
  
#if INTERNAL
  Global_show_cursor = true;
#else
  Global_show_cursor = false;
#endif
  
  HWND window_handle = create_default_window((LRESULT)win32_window_proc, current_instance, "GG", 
                                             initial_window_width, initial_window_height);
  
  macro_assert(window_handle);
  
  thread_context thread = {};
  
  // set refresh rate and target seconds per frame
  i32 refresh_rate = 60;
  HDC refresh_dc = GetDC(window_handle);
  i32 win32_refresh_rate = GetDeviceCaps(refresh_dc, VREFRESH);
  ReleaseDC(window_handle, refresh_dc);
  
  if (win32_refresh_rate > 1)
    refresh_rate = win32_refresh_rate;
  local_persist const i32 monitor_refresh_rate     = refresh_rate;
  local_persist const i32 game_update_refresh_rate = monitor_refresh_rate;
  
  f32 target_seconds_per_frame = 1.0f / (f32)game_update_refresh_rate;
  
  // sound stuff
  Win32_sound_output sound_output = {};
  i16* samples = 0;
  {
    sound_output.samples_per_second = 48000;
    sound_output.bytes_per_sample = sizeof(i16) * 2;
    sound_output.safety_bytes = i32((f32)sound_output.samples_per_second * (f32)sound_output.bytes_per_sample / game_update_refresh_rate / 3.0f);
    sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
    
    samples = (i16*)VirtualAlloc(0, sound_output.buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  }
  
  // memory allocation
  Game_memory memory = {};
  {
#if INTERNAL
    LPVOID base_address = (LPVOID)macro_terabytes(2);
#else
    LPVOID base_address = 0;
#endif
    
    memory.high_priority_queue = &queue;
    memory.platform_add_entry = win32_add_entry;
    memory.platform_complete_all_work = win32_complete_all_work;
    
    memory.permanent_storage_size = macro_megabytes(PERMANENT_MEMORY_SIZE_MB);
    memory.transient_storage_size = macro_megabytes(TRANSIENT_MEMORY_SIZE_MB);
    
    // consider trying MEM_LARGE_PAGES which would enable larger virtual memory page sizes (2mb? compared to 4k pages).
    // That would allow Translation lookaside buffer (TLB) (cpu table between physical memory and virtual) to work faster, maybe.
    win_state.total_memory_size = memory.permanent_storage_size + memory.transient_storage_size;
    win_state.game_memory_block = VirtualAlloc(base_address,
                                               (size_t)win_state.total_memory_size,
                                               MEM_RESERVE|MEM_COMMIT, 
                                               PAGE_READWRITE);
    
    memory.permanent_storage = win_state.game_memory_block;
    memory.transient_storage = (u8*)memory.permanent_storage + memory.permanent_storage_size;
    
    macro_assert(samples && memory.permanent_storage && memory.transient_storage);
  }
  
  win32_init_direct_sound(window_handle, sound_output.samples_per_second, sound_output.buffer_size);
  win32_clear_sound_buffer(&sound_output);
  Global_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
  
  // SIMD - single instruction multiple data
  auto xinput_ready = win32_load_xinput();
  
  Game_input input[2] = {};
  Game_input* new_input = &input[0];
  Game_input* old_input = &input[1];
  
  // sound stuff
  i32 debug_last_marker_index = 0;
  Win32_debug_time_marker debug_time_marker_list[15] = {};
  DWORD last_play_cursor = 0;
  DWORD last_write_cursor = 0;
  bool sound_first_pass = true;
  
#if INTERNAL
  DWORD cursor_bytes_delta;
  f32 audio_latency_seconds;
#endif
  
  last_counter = win32_get_wall_clock();
  flip_wall_clock = win32_get_wall_clock();
  auto begin_cycle_count = __rdtsc();
  
  while (Global_game_running) {
    
    new_input->executable_reloaded = false;
    new_input->time_delta = target_seconds_per_frame;
    
    // check if we need to reload game code
    {
      auto new_dll_write_time = win32_get_last_write_time(game_code_dll_name);
      
      if (CompareFileTime(&new_dll_write_time, &game_code.dll_last_write_time) != 0) {
        win32_unload_game_code(&game_code);
        
        game_code = win32_load_game_code(game_code_full_path, 
                                         temp_game_code_full_path, 
                                         lock_file_full_path);
        
        new_input->executable_reloaded = true;
      }
    }
    
    // input
    {
      auto old_keyboard_input = &old_input->gamepad[0];
      auto new_keyboard_input = &new_input->gamepad[0];
      *new_keyboard_input = {};
      
      for (i32 i = 0; i < macro_array_count(new_keyboard_input->buttons); i++) {
        new_keyboard_input->buttons[i].ended_down = old_keyboard_input->buttons[i].ended_down;
      }
      
      new_keyboard_input->is_connected = true;
      new_keyboard_input->is_analog = old_keyboard_input->is_analog;
      
      win32_handle_messages(&win_state, new_keyboard_input);
      
      POINT mouse_pos;
      GetCursorPos(&mouse_pos);
      // we need to call screen to client because mouse positions we're getting are global, but our window's position is not aligned
      ScreenToClient(window_handle, &mouse_pos);
      
      new_input->mouse_x = mouse_pos.x;
      new_input->mouse_y = mouse_pos.y;
      
      bool show_mouse_coords = false;
      
      if (show_mouse_coords) {
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "x: %d y: %d \n", mouse_pos.x, mouse_pos.y);
        OutputDebugStringA(buffer);
      }
      
      win32_process_keyboard_input(&new_input->mouse_buttons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
      win32_process_keyboard_input(&new_input->mouse_buttons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
      win32_process_keyboard_input(&new_input->mouse_buttons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
    }
    
    // managing controller
    if (xinput_ready) {
      DWORD max_gamepad_count = XUSER_MAX_COUNT;
      if (max_gamepad_count > macro_array_count(new_input->gamepad) - 1) {
        max_gamepad_count = macro_array_count(new_input->gamepad) - 1;
      }
      
      for (DWORD i = 0; i < max_gamepad_count; i++) {
        auto our_controller_index = i + 1;
        auto old_gamepad = get_gamepad(old_input, our_controller_index);
        auto new_gamepad = get_gamepad(new_input, our_controller_index);
        new_gamepad->is_analog = false;
        
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        
        // Simply get the state of the controller from XInput.
        auto dwResult = XInputGetState(i, &state);
        
        if (dwResult == ERROR_SUCCESS) {
          new_gamepad->is_connected = true;
          // Controller is connected
          auto* pad = &state.Gamepad;
          
          // button is enabled if pad->wButton dword (32 bits - 4 bytes) and (&) with some bytes (0x0001 0x0002 ..)
          bool button_left_thumb     = (pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? true : false;
          bool button_right_thumb    = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? true : false;
          bool button_left_shoulder  = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? true : false;
          bool button_right_shoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? true : false;
          
          new_gamepad->stick_avg_y = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP    ? 1.0f  : 0.0f;
          new_gamepad->stick_avg_x = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ? 1.0f  : 0.0f;
          new_gamepad->stick_avg_y = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN  ? -1.0f : 0.0f;
          new_gamepad->stick_avg_x = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT  ? -1.0f : 0.0f;
          
          new_gamepad->stick_avg_x = win32_xinput_cutoff_deadzone(pad->sThumbLX);
          new_gamepad->stick_avg_y = win32_xinput_cutoff_deadzone(pad->sThumbLY);
          
          if (new_gamepad->stick_avg_x != 0.0f || new_gamepad->stick_avg_y != 0.0f)
            new_gamepad->is_analog = true;
          
          auto threshold = 0.5f;
          win32_process_xinput_button((new_gamepad->stick_avg_x < -threshold) ? 1 : 0, 1, 
                                      &old_gamepad->left, &new_gamepad->left);
          
          win32_process_xinput_button((new_gamepad->stick_avg_x > threshold)  ? 1 : 0, 1, 
                                      &old_gamepad->right, &new_gamepad->right);
          
          win32_process_xinput_button((new_gamepad->stick_avg_y < -threshold) ? 1 : 0, 1, 
                                      &old_gamepad->down, &new_gamepad->down);
          
          win32_process_xinput_button((new_gamepad->stick_avg_y > threshold)  ? 1 : 0, 1, 
                                      &old_gamepad->up, &new_gamepad->up);
          
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_DPAD_UP,    &old_gamepad->up,    &new_gamepad->up);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, &old_gamepad->right, &new_gamepad->right);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN,  &old_gamepad->down,  &new_gamepad->down);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT,  &old_gamepad->left,  &new_gamepad->left);
          
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_START, &old_gamepad->start, &new_gamepad->start);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_BACK,  &old_gamepad->back,  &new_gamepad->back);
          
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_A, &old_gamepad->cross_or_a, &new_gamepad->cross_or_a);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_B, &old_gamepad->circle_or_b, &new_gamepad->circle_or_b);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_X, &old_gamepad->box_or_x,  &new_gamepad->box_or_x);
          win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_Y, &old_gamepad->triangle_or_y, &new_gamepad->triangle_or_y);
          
          XINPUT_VIBRATION vibration;
          vibration.wLeftMotorSpeed = 60000;
          vibration.wRightMotorSpeed = 60000;
          XInputSetState(i, &vibration); 
        } else {
          new_gamepad->is_connected = false;
          // TODO: log: Controller is not connected
        }
      }
    }
    swap(new_input, old_input);
    // end input
    
    // record, playback, update and render
    {
      Game_bitmap_buffer game_buffer = {};
      game_buffer.memory = Global_backbuffer.memory;
      game_buffer.width = Global_backbuffer.width;
      game_buffer.height = Global_backbuffer.height;
      game_buffer.pitch = Global_backbuffer.pitch;
      game_buffer.window_width = initial_window_width;
      game_buffer.window_height = initial_window_height;
      
      if (win_state.recording_input_index)
        win32_record_input(&win_state, new_input);
      
      if (win_state.playing_input_index)
        win32_playback_input(&win_state, new_input);
      
      if (game_code.update_and_render) {
        game_code.update_and_render(&thread, &memory, new_input, &game_buffer);
        handle_debug_cycle_count(&memory);
      }
    }
    
    // sound stuff
    {
      DWORD bytes_to_lock, bytes_to_write, target_cursor, play_cursor, write_cursor;
      bytes_to_lock = bytes_to_write = target_cursor = play_cursor = write_cursor = 0;
      Game_sound_buffer sound_buffer = {};
      LARGE_INTEGER audio_wallclock = win32_get_wall_clock();
      f32 from_begin_to_audio_seconds = win32_get_seconds_elapsed(flip_wall_clock, audio_wallclock);
      
      if (Global_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor) != DS_OK) {
        goto failed_sound_exit;
      }
      
      if (sound_first_pass) {
        sound_output.running_sample_index = write_cursor / sound_output.bytes_per_sample;
        sound_first_pass = false;
      }
      
      bytes_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size;
      
      DWORD expected_sound_bytes_per_frame = (i32)((sound_output.samples_per_second * sound_output.bytes_per_sample) / game_update_refresh_rate);
      
      f32 seconds_left_until_flip = target_seconds_per_frame - from_begin_to_audio_seconds;
      
      DWORD expected_bytes_until_flip = (DWORD)(seconds_left_until_flip / target_seconds_per_frame) * expected_sound_bytes_per_frame;
      
      DWORD expected_frame_boundry_byte = play_cursor + expected_bytes_until_flip;
      
      DWORD safe_write_cursor = write_cursor;
      
      if (safe_write_cursor < play_cursor)
        safe_write_cursor += sound_output.buffer_size;
      
      macro_assert(safe_write_cursor >= play_cursor);
      
      safe_write_cursor += sound_output.safety_bytes;
      bool audio_is_low_latency = safe_write_cursor < expected_frame_boundry_byte;
      
      if (audio_is_low_latency)
        target_cursor = expected_frame_boundry_byte + expected_sound_bytes_per_frame;
      else
        target_cursor = write_cursor + expected_sound_bytes_per_frame + sound_output.safety_bytes;
      
      target_cursor %= sound_output.buffer_size;
      
      // if we have two chunks to write (play cursor is further than "bytes to write" cursor
      if (bytes_to_lock > target_cursor) {
        bytes_to_write = sound_output.buffer_size - bytes_to_lock;
        bytes_to_write += target_cursor;
      } else {
        bytes_to_write = target_cursor - bytes_to_lock;
      }
      
      // i16 samples[48000 * 2];
      sound_buffer.samples_per_second = sound_output.samples_per_second;
      sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
      sound_buffer.samples = samples;
      
      if (game_code.get_sound_samples)
        game_code.get_sound_samples(&thread, &memory, &sound_buffer);
      
#if INTERNAL
      auto marker = &debug_time_marker_list[debug_last_marker_index];
      marker->output_play_cursor = play_cursor;
      marker->output_write_cursor = write_cursor;
      marker->output_location = bytes_to_lock;
      marker->output_bytes = bytes_to_write;
      marker->expected_play_cursor = expected_frame_boundry_byte;
      
      DWORD temp_play_cursor, temp_write_cursor;
      Global_sound_buffer->GetCurrentPosition(&temp_play_cursor, &temp_write_cursor);
      
      // audio latency in bytes
      cursor_bytes_delta = abs((i32)temp_write_cursor - (i32)temp_play_cursor);
      audio_latency_seconds = ((f32)cursor_bytes_delta / (f32)sound_output.bytes_per_sample) / (f32)sound_output.samples_per_second;
      
#if 0
      char buffer[256];
      _snprintf_s(buffer, sizeof(buffer),  
                  "play cursor: %u; byte to lock: %u; target cursor: %u; bytes_to_write: %u; delta: %u; t play cursor: %u; t write cursor: %u; latency: %f s\n", 
                  last_play_cursor, bytes_to_lock, target_cursor, bytes_to_write, cursor_bytes_delta, temp_play_cursor, temp_write_cursor, audio_latency_seconds);
      OutputDebugStringA(buffer);
#endif
      
      // display debug cursors
      bool draw_debug_sound_markers = false;
      
      if (draw_debug_sound_markers)
        win32_debug_sync_display(&Global_backbuffer, 
                                 macro_array_count(debug_time_marker_list), debug_last_marker_index - 1,
                                 debug_time_marker_list, &sound_output, target_seconds_per_frame);
      
#endif
      goto exit;
      
      failed_sound_exit:
#if DEBUG
      OutputDebugStringA("Failed to get current sound position\n");
#endif
      exit:
      win32_fill_sound_buffer(&sound_output, bytes_to_lock, bytes_to_write, &sound_buffer);
    }
    
    // ensuring stable fps
    {
      auto elapsed_s = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
      
      if (elapsed_s < target_seconds_per_frame) {
        if (sleep_is_granular) {
          auto sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - elapsed_s));
          if (sleep_ms > 0) 
            Sleep(sleep_ms);
        }
        
        while (elapsed_s < target_seconds_per_frame) {
          elapsed_s = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
        }
      } else {
        // missing frames
      }
      
      auto end_cycle_count = __rdtsc();
      end_counter = win32_get_wall_clock();
      
      // draw
      auto dimensions = get_window_dimensions(window_handle);
      HDC device_context = GetDC(window_handle);
      win32_display_buffer_to_window(&Global_backbuffer, device_context, dimensions.width, dimensions.height);
      ReleaseDC(window_handle, device_context);
      
      flip_wall_clock = win32_get_wall_clock();
#if INTERNAL
      {
        DWORD temp_play_cursor, temp_write_cursor;
        if (Global_sound_buffer->GetCurrentPosition(&temp_play_cursor, &temp_write_cursor) == DS_OK) {
          macro_assert(debug_last_marker_index < macro_array_count(debug_time_marker_list));
          auto marker = &debug_time_marker_list[debug_last_marker_index];
          marker->play_cursor = temp_play_cursor;
          marker->write_cursor = temp_write_cursor;
        }
        
        debug_last_marker_index++;
        if (debug_last_marker_index == macro_array_count(debug_time_marker_list)) {
          debug_last_marker_index = 0;
        }
      }
#endif
      
      bool output_to_debug_fps = true;
      if (output_to_debug_fps) {
        auto cycles_elapsed = (u32)(end_cycle_count - begin_cycle_count);
        auto counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
        
        auto fps = (i32)(Global_perf_freq / counter_elapsed);
        auto elapsed_ms = (i32)(counter_elapsed * 1000.0f / Global_perf_freq);
        
        char buffer[256];
        _snprintf_s(buffer, sizeof(buffer), "%d ms/f  %d f/s  %d MC/f \n", elapsed_ms, fps, cycles_elapsed / 1000000);
        // approx. cpu speed - fps * (cycles_elapsed / 1000000)
        OutputDebugStringA(buffer);
      }
      
      last_counter = end_counter;
      begin_cycle_count = end_cycle_count;
    }
  }
  
  CloseHandle(queue.semaphore);
  return 0;
}
#pragma optimize("", on) 