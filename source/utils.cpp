#include <stdint.h>
#include "types.h"

internal
void
win32_debug_msg(char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    auto count = vsprintf_s((char*)buffer, macro_array_count(buffer), format, args); 
    OutputDebugStringA((char*)buffer);
}

template <typename T> 
void swap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

void 
string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest) {
    for (u32 i = 0; i < src_a_count; i++) {
        *dest++ = *src_a++;
    }
    
    for (u32 i = 0; i < src_b_count; i++) {
        *dest++ = *src_b++;
    }
    
    *dest++ = 0;
}

i32 
string_len(char* string) {
    i32 result = 0;
    
    // *string dereference value
    // string++ advances poi32er
    // search for null terminator
    while (*string++ != 0) {
        result++;
    }
    
    return result;
}

i32 
string_to_binary(char* str) {
	i32 size = string_len(str);
	i32 result = 0;
	char* ptr = str;
	i32 i = 0;
    
	while (*ptr) {
		i++;
		if (*ptr == '1') {
			result |= (1 << (size - i));
		}
		ptr++;
	}
    
	return result;
}

char* 
i32_to_string(i32 n) {
	local_persist char binary[8] = {};
	for (auto x = 0; x < 8; x++) {
		binary[x] = n & 0x80 ? '1' : '0';
		n <<= 1;
	}
	return binary;
}

i32 
clamp_i32(i32 val, i32 min_value = 0, i32 max_value = 1) {
    i32 result = val;
    
    if (val < min_value)
        result = min_value;
    
    if (val > max_value)
        result = max_value;
    
    return result;
}

u32 
clamp_u32(u32 val, u32 min_value = 0, u32 max_value = 1) {
    u32 result = val;
    
    if (val < min_value)
        result = min_value;
    
    if (val > max_value)
        result = max_value;
    
    return result;
}

f32 
clamp_f32(f32 val, f32 min_value = 0.0f, f32 max_value = 1.0f) {
    f32 result = val;
    
    if (val < min_value)
        result = min_value;
    
    if (val > max_value)
        result = max_value;
    
    return result;
}