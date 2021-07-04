#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
// https://www.youtube.com/watch?v=tAcUIEoy2Yk

#define PI 3.14159265358979323846f

struct win32_bitmap_buffer {
    // pixels are always 32 bit, memory order BB GG RR XX (padding)
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct win32_window_dimensions {
    int width;
    int height;
};

struct win32_sound_output {
    int samples_per_second;
    int tone_hz;
    int wave_period;
    int latency_sample_count;
    float sine_val;
    int16_t tone_volume;
    int32_t bytes_per_sample;
    int32_t buffer_size;
    uint32_t running_sample_index;
};

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

static void win32_fill_sound_buffer(win32_sound_output* sound_output, DWORD bytes_to_lock, DWORD bytes_to_write) {
    void* region_one;
    DWORD region_one_size;
    void* region_two;
    DWORD region_two_size;
    
    if (!SUCCEEDED(Global_sound_buffer->Lock(bytes_to_lock, bytes_to_write, &region_one, &region_one_size, &region_two, &region_two_size, 0))) {
        // too much fails OutputDebugStringA("Failed to lock global sound buffer\n");
        return;
    }
    
    auto sample_out = (int16_t*)region_one;
    DWORD region_sample_count = region_one_size / sound_output->bytes_per_sample;
    
    for (DWORD sample_index = 0; sample_index < region_sample_count; sample_index++) {
        float sine_val = sinf(sound_output->sine_val);
        int16_t sample_value = (int16_t)(sine_val * sound_output->tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        sound_output->sine_val += PI * 2.0f * ((float)1.0 / (float)sound_output->wave_period);
        sound_output->running_sample_index++;
    }
    
    sample_out = (int16_t*)region_two;
    DWORD region_sample_count_two = region_two_size / sound_output->bytes_per_sample;
    
    for (DWORD sample_index = 0; sample_index < region_sample_count_two; sample_index++) {
        float sine_val = sinf(sound_output->sine_val);
        int16_t sample_value = (int16_t)(sine_val * sound_output->tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        
        sound_output->sine_val += PI * 2.0f * ((float)1.0f / (float)sound_output->wave_period);
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

static void render_255_gradient(win32_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {
    
    auto row = (uint8_t*)bitmap_buffer->memory;
    
    for (int y = 0; y < bitmap_buffer->height; y++) {
        auto pixel = (uint32_t*)row;
        for (int x = 0; x < bitmap_buffer->width; x++) {
            // pixel bytes	   1  2	 3  4
            // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
            // little endian
            
            uint8_t blue = x + blue_offset;
            uint8_t green = y + green_offset;
            
            // 0x 00 00 00 00 -> 0x xx rr gg bb
            // | composites bytes
            // green << 8 - shifts by 8 bits
            // other stay 00
            // * dereference pixel
            // pixel++ - pointer arithmetic - jumps by it's size (32 bits in this case)
            *pixel++ = (green << 8) | blue;
        }
        
        row += bitmap_buffer->pitch;
    }
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
    int x_offset = 0, y_offset = 0;
    
    // sound stuff
    win32_sound_output sound_output = {};
    sound_output.samples_per_second = 48000;
    sound_output.tone_hz = 256;
    sound_output.tone_volume = 3000;
    sound_output.bytes_per_sample = sizeof(int16_t) * 2;
    sound_output.latency_sample_count = sound_output.samples_per_second / 15;
    sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
    sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
    
    win32_init_direct_sound(window_handle, sound_output.samples_per_second, sound_output.buffer_size);
    win32_fill_sound_buffer(&sound_output, 0, sound_output.latency_sample_count*sound_output.bytes_per_sample);
    Global_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
    
    while (Global_game_running) {
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
            
            if (message.message == WM_QUIT) Global_game_running = false;
            
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
        
        // managing controller
        if (xinput_ready) {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));
                
                // Simply get the state of the controller from XInput.
                auto dwResult = XInputGetState(i, &state);
                
                if (dwResult == ERROR_SUCCESS) {
                    sound_output.tone_hz = 256;
                    sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
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
                    auto button_a = pad->wButtons & XINPUT_GAMEPAD_A;
                    auto button_b = pad->wButtons & XINPUT_GAMEPAD_B;
                    auto button_x = pad->wButtons & XINPUT_GAMEPAD_X;
                    auto button_y = pad->wButtons & XINPUT_GAMEPAD_Y;
                    
                    auto stick_x = pad->sThumbLX;
                    auto stick_y = pad->sThumbLY;
                    
                    x_offset += stick_x / 4096;
                    y_offset -= stick_y / 4096;
                    
                    sound_output.tone_hz = 512 + (int)(256.0f*((float)stick_y / 30000.f));
                    sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
                    
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
        
        render_255_gradient(&Global_backbuffer, x_offset, y_offset);
        
        deviceContext = GetDC(window_handle);
        
        auto dimensions = get_window_dimensions(window_handle);
        win32_display_buffer_to_window(&Global_backbuffer, deviceContext, dimensions.width, dimensions.height, 0, 0, dimensions.width, dimensions.height);
        
        // play sound
        {
            DWORD play_cursor;
            DWORD write_cursor;
            
            if (!SUCCEEDED(Global_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
                OutputDebugStringA("Failed to get current position\n");
                return -1;
            } 
            
            DWORD bytes_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size;
            DWORD bytes_to_write;
            DWORD target_cursor = (play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.buffer_size;
            
            // if we have two chunks to write (play cursor is further than "bytes to write" cursor
            if (bytes_to_lock > target_cursor) {
                bytes_to_write = sound_output.buffer_size - bytes_to_lock;
                bytes_to_write += target_cursor;
            } else {
                bytes_to_write = target_cursor - bytes_to_lock;
            }
            
            win32_fill_sound_buffer(&sound_output, bytes_to_lock, bytes_to_write);
        }
        
        ReleaseDC(window_handle, deviceContext);
    }
    
    return 0;
}
