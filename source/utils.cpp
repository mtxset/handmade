#include <stdint.h>
#include "types.h"

template <typename T> void 
swap(T& a, T& b) {
    T c(a);
    a = b;
    b = c;
}

u32 
truncate_u64(u64 value) {
    macro_assert(value <= UINT32_MAX);
    return (u32)value;
}

void
string_concat(size_t src_a_count, char* src_a, size_t src_b_count, char* src_b, size_t dest_count, char* dest) {
    for (int i = 0; i < src_a_count; i++) {
        *dest++ = *src_a++;
    }
    
    for (int i = 0; i < src_b_count; i++) {
        *dest++ = *src_b++;
    }
    
    *dest++ = 0;
}

int 
string_len(char* string) {
    int result = 0;
    
    // *string dereference value
    // string++ advance pointer
    // search for null terminator
    while (*string++ != 0) {
        result++;
    }
    
    return result;
}