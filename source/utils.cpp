#include <stdint.h>
#include "types.h"

template <typename T> 
void swap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

u32 truncate_u64(u64 value) {
    macro_assert(value <= UINT32_MAX);
    return (u32)value;
}

void string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest) {
    for (int i = 0; i < src_a_count; i++) {
        *dest++ = *src_a++;
    }
    
    for (int i = 0; i < src_b_count; i++) {
        *dest++ = *src_b++;
    }
    
    *dest++ = 0;
}

int string_len(char* string) {
    int result = 0;
    
    // *string dereference value
    // string++ advances pointer
    // search for null terminator
    while (*string++ != 0) {
        result++;
    }
    
    return result;
}

int string_to_binary(char* str) {
	int size = string_len(str);
	int result = 0;
	char* ptr = str;
	int i = 0;
    
	while (*ptr) {
		i++;
		if (*ptr == '1') {
			result |= (1 << (size - i));
		}
		ptr++;
	}
    
	return result;
}

char* int_to_string(int n) {
	static char binary[8];
	for (auto x = 0; x < 8; x++) {
		binary[x] = n & 0x80 ? '1' : '0';
		n <<= 1;
	}
	return(binary);
}

i32 round_f32_i32(f32 value) {
    i32 result;
    
    result = (i32)(value + 0.5f);
    // 0.5 cuz c will truncate decimal points, so 0.6 will be 0
    
    return result;
}

u32 round_f32_u32(f32 value) {
    u32 result;
    
    result = (u32)(value + 0.5f);
    
    return result;
}