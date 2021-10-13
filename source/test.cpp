#include "intrinsics.h"

void rotate_tests() {
    u32 a = 0x12345678;
    u32 actual_left4_pos = rotate_left(a, 4);
    macro_assert(actual_left4_pos == 0x23456781);
    
    u32 actual_left4_neg = rotate_left(a, -4);
    macro_assert(actual_left4_neg == 0x81234567);
    
    u32 actual_right4_pos = rotate_right(a, 4);
    macro_assert(actual_right4_pos == 0x81234567);
    
    u32 actual_right4_neg = rotate_right(a, -4);
    macro_assert(actual_right4_neg == 0x23456781);
}

void run_tests() {
    rotate_tests();
}
