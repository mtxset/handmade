// https://youtu.be/Lt9DfMzZ9sI?t=3940
#define PI 3.14159265358979323846f

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <malloc.h>
#include "main.h"
#include "game.cpp"

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

static auto Global_game_running = true;

static win32_bitmap_buffer Global_backbuffer;
static HBITMAP Global_bitmap_handle;
static HDC Global_bitmap_device_context;
static LPDIRECTSOUNDBUFFER Global_sound_buffer;

// making sure that if we don't have links to functions we don't crash because we use stubs
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);            // test 
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);    // test
static x_input_get_state* XInputGetState_;
static x_input_set_state* XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

typedef HRESULT WINAPI direct_sound_create(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter); // define delegate
static direct_sound_create* DirectSoundCreate_;                                                             // define variable to hold it
#define DirectSoundCreate DirectSoundCreate_                                                                // change name by which we reference upper-line mentioned variable

static void win32_init_direct_sound(HWND window, int32_t samples_per_second, int32_t buffer_size) {
    // NOTE: Load the library
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
    
    if (DSoundLibrary) {
        // NOTE: Get a DirectSound Object
        direct_sound_create* DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
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
            buffer_desc.dwFlags = 0;
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

static void win32_clear_sound_buffer(win32_sound_output* sound_output) {
    void* region_one;
    DWORD region_one_size;
    void* region_two;
    DWORD region_two_size;
    
    if (!SUCCEEDED(Global_sound_buffer->Lock(0, sound_output->buffer_size, &region_one, &region_one_size, &region_two, &region_two_size, 0))) {
        // too much fails OutputDebugStringA("Failed to lock global sound buffer\n");
        return;
    }
    
    auto sample_out = (int8_t*)region_one;
    for (DWORD index = 0; index < region_one_size; index++) {
        *sample_out++ = 0;
    }
    
    sample_out = (int8_t*)region_two;
    for (DWORD index = 0; index < region_two_size; index++) {
        *sample_out++ = 0;
    }
    
    Global_sound_buffer->Unlock(region_one, region_one_size, region_two, region_two_size);
    
}

static void win32_fill_sound_buffer(win32_sound_output* sound_output, DWORD bytes_to_lock, DWORD bytes_to_write, game_sound_buffer* source_buffer) {
    void* region_one;
    DWORD region_one_size;
    void* region_two;
    DWORD region_two_size;
    
    if (!SUCCEEDED(Global_sound_buffer->Lock(bytes_to_lock, bytes_to_write, &region_one, &region_one_size, &region_two, &region_two_size, 0))) {
        // too much fails OutputDebugStringA("Failed to lock global sound buffer\n");
        return;
    }
    
    auto sample_out = (int16_t*)region_one;
    auto source_out = (int16_t*)source_buffer->samples;
    DWORD region_sample_count = region_one_size / sound_output->bytes_per_sample;
    
    for (DWORD sample_index = 0; sample_index < region_sample_count; sample_index++) {
        *sample_out++ = *source_out++;
        *sample_out++ = *source_out++;
        
        sound_output->running_sample_index++;
    }
    
    sample_out = (int16_t*)region_two;
    DWORD region_sample_count_two = region_two_size / sound_output->bytes_per_sample;
    
    for (DWORD sample_index = 0; sample_index < region_sample_count_two; sample_index++) {
        *sample_out++ = *source_out++;
        *sample_out++ = *source_out++;
        
        sound_output->running_sample_index++;
    }
    
    Global_sound_buffer->Unlock(region_one, region_one_size, region_two, region_two_size);
}

static bool win32_load_xinput() {
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

static win32_window_dimensions get_window_dimensions(HWND window) {
    
    win32_window_dimensions result;
    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    
    result.height = clientRect.bottom - clientRect.top;
    result.width = clientRect.right - clientRect.left;
    
    return result;
}

// DIB - device independant section
static void win32_resize_dib_section(win32_bitmap_buffer* bitmap_buffer, int width, int height) {
    
    if (bitmap_buffer->memory) {
        VirtualFree(bitmap_buffer->memory, 0, MEM_RELEASE);
    }
    
    bitmap_buffer->width = width;
    bitmap_buffer->height = height;
    bitmap_buffer->bytes_per_pixel = 4;
    bitmap_buffer->pitch = bitmap_buffer->bytes_per_pixel * width;
    
    bitmap_buffer->info.bmiHeader.biSize = sizeof(bitmap_buffer->info.bmiHeader);
    bitmap_buffer->info.bmiHeader.biWidth = width;
    bitmap_buffer->info.bmiHeader.biHeight = -height; // draw left-to-right, top-to-bottom
    bitmap_buffer->info.bmiHeader.biPlanes = 1;
    bitmap_buffer->info.bmiHeader.biBitCount = 32;
    bitmap_buffer->info.bmiHeader.biCompression = BI_RGB;
    
    // we are taking 4 bytes (8 bits for each color (rgb) + aligment 8 bits = 32 bits) for each pixel
    auto bitmap_memory_size = width * height * bitmap_buffer->bytes_per_pixel;
    // virtual alloc allocates region of page which, one page size is 1 mb? GetSystemInfo can output that info
    bitmap_buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

inline static void win32_display_buffer_to_window(win32_bitmap_buffer* bitmap_buffer, HDC deviceContext, int window_width, int window_height, int x, int y, int width, int height) {
    // StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(deviceContext, 0, 0, window_width, window_height, 0, 0, bitmap_buffer->width, bitmap_buffer->height, bitmap_buffer->memory, &bitmap_buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

static void win32_process_xinput_button(DWORD xinput_button_state, DWORD button_bit, game_button_state* new_state, game_button_state* old_state) {
    new_state->ended_down = (xinput_button_state & button_bit) == button_bit;
    new_state->half_transition_count = old_state->ended_down != new_state->ended_down ? 1 : 0;
}

LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch (message) {
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN: {
            auto vk_key = wParam;
            auto previous_state = lParam & (1 << 30);        // will return 0 or bit 30
            bool was_down       = (lParam & (1 << 30)) != 0; // if I get 0 I get true if I get something besides zero I compare it to zero and I will get false
            bool is_down        = (lParam & (1 << 31)) == 0; // parenthesis required because == has precedence over &
            bool alt_is_down    = (lParam & (1 << 29)) != 0; // will return 0 or bit 29; if I get 29 alt is down - if 0 it's not so I compare it to 0
            
            if (vk_key == 'W') {
            } else if (vk_key == 'S') {
            } else if (vk_key == 'A') {
            } else if (vk_key == 'D') {
            } else if (vk_key == VK_UP) {
            } else if (vk_key == VK_LEFT) {
            } else if (vk_key == VK_DOWN) {
            } else if (vk_key == VK_RIGHT) {
            } else if (vk_key == VK_ESCAPE) {
            } else if (vk_key == VK_SPACE) {
            } else if (vk_key == VK_F4 && alt_is_down) {
                Global_game_running = false;
            }
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
            auto deviceContext = BeginPaint(window, &paint);
            
            auto x = paint.rcPaint.left;
            auto y = paint.rcPaint.top;
            auto width = paint.rcPaint.right - paint.rcPaint.left;
            auto height = paint.rcPaint.bottom - paint.rcPaint.top;
            
            auto dimensions = get_window_dimensions(window);
            win32_display_buffer_to_window(&Global_backbuffer, deviceContext, dimensions.width, dimensions.height, x, y, width, height);
            EndPaint(window, &paint);
        } break;
        
        default: {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }
    
    return result;
}

int main(HINSTANCE currentInstance, HINSTANCE previousInstance, LPSTR commandLineParams, int nothing) {
    
    auto xinput_ready = win32_load_xinput();
    
    WNDCLASSA window_class = {};
    
    win32_resize_dib_section(&Global_backbuffer, 1280, 720);
    
    window_class.style = CS_HREDRAW | CS_HREDRAW; // redraw full window (vertical/horizontal) (streches)
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = currentInstance;
    //WindowClass.hIcon = 0;
    window_class.lpszClassName = "GG";
    
    if (RegisterClass(&window_class) == 0) {
        OutputDebugStringA("RegisterClass failed\n");
        return -1;
    }
    
    auto window_handle = CreateWindowEx(0, window_class.lpszClassName, "GG", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, currentInstance, 0);
    
    if (window_handle == 0) {
        OutputDebugStringA("CreateWindow failed\n");
        return -1;
    }
    
    MSG message;
    HDC deviceContext;
    
    // sound stuff
    win32_sound_output sound_output = {};
    sound_output.samples_per_second = 48000;
    sound_output.bytes_per_sample = sizeof(int16_t) * 2;
    sound_output.latency_sample_count = sound_output.samples_per_second / 15;
    sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
    
    auto samples = (int16_t*)VirtualAlloc(0, sound_output.buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    win32_init_direct_sound(window_handle, sound_output.samples_per_second, sound_output.buffer_size);
    win32_clear_sound_buffer(&sound_output);
    Global_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
    
    LARGE_INTEGER performance_freq, start_counter, end_counter, elapsed_counter;
    QueryPerformanceFrequency(&performance_freq);
    QueryPerformanceCounter(&start_counter);
    
    auto begin_cycle_count = __rdtsc();
    // SIMD - single instruction multiple data
    game_input input[2] = {};
    game_input* new_input = &input[0];
    game_input* old_input = &input[1];
    
    while (Global_game_running) {
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            
            if (message.message == WM_QUIT) Global_game_running = false;
            
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        // managing controller
        if (xinput_ready) {
            DWORD max_gamepad_count = XUSER_MAX_COUNT;
            if (max_gamepad_count > macro_array_count(new_input->gamepad))
            {
                max_gamepad_count = macro_array_count(new_input->gamepad);
            }
            
            for (DWORD i = 0; i < max_gamepad_count; i++) {
                auto old_gamepad = &old_input->gamepad[i];
                auto new_gamepad = &new_input->gamepad[i];
                
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));
                
                // Simply get the state of the controller from XInput.
                auto dwResult = XInputGetState(i, &state);
                
                if (dwResult == ERROR_SUCCESS) {
                    // Controller is connected
                    auto* pad = &state.Gamepad;
                    
                    // button is enabled if pad->wButton dword (32 bits - 4 bytes) and (&) with some bytes (0x0001 0x0002 ..)
                    auto button_up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                    auto button_down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                    auto button_left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                    auto button_right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                    
                    auto button_start = pad->wButtons & XINPUT_GAMEPAD_START;
                    auto button_back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                    auto button_left_thumb  = pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                    auto button_right_thumb = pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                    auto button_left_shoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                    auto button_right_shoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                    //auto button_a = pad->wButtons & XINPUT_GAMEPAD_A;
                    //auto button_b = pad->wButtons & XINPUT_GAMEPAD_B;
                    //auto button_x = pad->wButtons & XINPUT_GAMEPAD_X;
                    //auto button_y = pad->wButtons & XINPUT_GAMEPAD_Y;
                    
                    new_gamepad->start_x = old_gamepad->start_x;
                    new_gamepad->start_y = old_gamepad->start_y;
                    new_gamepad->is_analog = true;
                    
                    float x;
                    if (pad->sThumbLX > 0)
                        x = pad->sThumbLX / 32767.0f;
                    else
                        x = pad->sThumbLX / 32768.0f;
                    new_gamepad->min_x = new_gamepad->max_x = new_gamepad->end_x = x;
                    
                    float y;
                    if (pad->sThumbLY > 0)
                        y = pad->sThumbLY / 32767.0f;
                    else
                        y = pad->sThumbLY / 32768.0f;
                    new_gamepad->min_y = new_gamepad->max_y = new_gamepad->end_y = y;
                    
                    auto stick_x = pad->sThumbLX;
                    auto stick_y = pad->sThumbLY;
                    
                    win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_A, &old_gamepad->up, &new_gamepad->up);
                    win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_B, &old_gamepad->right, &new_gamepad->right);
                    win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_X, &old_gamepad->left, &new_gamepad->left);
                    win32_process_xinput_button(pad->wButtons, XINPUT_GAMEPAD_Y, &old_gamepad->up, &new_gamepad->up);
                    
                    XINPUT_VIBRATION vibration;
                    vibration.wLeftMotorSpeed = 60000;
                    vibration.wRightMotorSpeed = 60000;
                    XInputSetState(i, &vibration); 
                }
                else {
                    // TODO: log: Controller is not connected
                }
            }
        }
        
        // update
        game_bitmap_buffer game_buffer = {};
        game_buffer.memory = Global_backbuffer.memory;
        game_buffer.width = Global_backbuffer.width;
        game_buffer.height = Global_backbuffer.height;
        game_buffer.pitch = Global_backbuffer.pitch;
        game_buffer.bytes_per_pixel = Global_backbuffer.bytes_per_pixel;
        
        deviceContext = GetDC(window_handle);
        
        auto dimensions = get_window_dimensions(window_handle);
        win32_display_buffer_to_window(&Global_backbuffer, deviceContext, dimensions.width, dimensions.height, 0, 0, dimensions.width, dimensions.height);
        ReleaseDC(window_handle, deviceContext);
        
        // play sound
        DWORD bytes_to_lock, bytes_to_write, target_cursor, play_cursor, write_cursor;
        bytes_to_lock = bytes_to_write = target_cursor = play_cursor = write_cursor = 0;
        auto sound_is_valid = false;
        
        if (SUCCEEDED(Global_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
            bytes_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size;
            target_cursor = (play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.buffer_size;
            
            // if we have two chunks to write (play cursor is further than "bytes to write" cursor
            if (bytes_to_lock > target_cursor) {
                bytes_to_write = sound_output.buffer_size - bytes_to_lock;
                bytes_to_write += target_cursor;
            } else {
                bytes_to_write = target_cursor - bytes_to_lock;
            }
            sound_is_valid = true;
        } 
        
        
        // int16_t samples[48000 * 2];
        game_sound_buffer sound_buffer = {};
        sound_buffer.samples_per_second = sound_output.samples_per_second;
        sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
        sound_buffer.samples = samples;
        
        if (sound_is_valid) {
            win32_fill_sound_buffer(&sound_output, bytes_to_lock, bytes_to_write, &sound_buffer);
        }
        
        game_update_render(new_input, &game_buffer, &sound_buffer);
        // clock
        
        QueryPerformanceCounter(&end_counter);
        
        elapsed_counter.QuadPart = end_counter.QuadPart - start_counter.QuadPart;
        auto elapsed_ms = (int32_t)((1000 * elapsed_counter.QuadPart) / performance_freq.QuadPart);
        auto fps = (int32_t)(performance_freq.QuadPart / elapsed_counter.QuadPart);
        
        auto end_cycle_count = __rdtsc();
        auto cycles_elapsed = (uint32_t)(end_cycle_count - begin_cycle_count);
        
        char buffer[256];
        wsprintf(buffer, "%d ms/f; fps: %d, megacycles/f: %d \n", elapsed_ms, fps, cycles_elapsed / 1000000);
        OutputDebugStringA(buffer);
        // approx. cpu speed - fps * (cycles_elapsed / 1000000)
        
        start_counter = end_counter;
        begin_cycle_count = end_cycle_count;
        
        auto temp = new_input;
        new_input = old_input;
        old_input = temp;
    }
    
    return 0;
}