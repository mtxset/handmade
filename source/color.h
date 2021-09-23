/* date = September 22nd 2021 11:02 am */

#ifndef COLOR_H
#define COLOR_H

struct color_f32 {
    f32 r;
    f32 g;
    f32 b;
};

global_var const u32 color_black_byte  = 0xf0000000;
global_var const u32 color_white_byte  = 0xffffffff;
global_var const u32 color_purple_byte = 0x00ff00ff;
global_var const u32 color_cyan_byte   = 0x0000ffff;
global_var const u32 color_red_byte    = 0x00ff0000;
global_var const u32 color_green_byte  = 0x0000ff00;
global_var const u32 color_blue_byte   = 0x000000ff;
global_var const u32 color_gold_byte   = 0x00b48c06;
global_var const u32 color_yellow_byte = 0x00ffff00;
global_var const u32 color_gray_byte   = 0x0075787b;

global_var const color_f32 COLOR_GOLD = { 1.0f, 0.8f, .0f };

#endif //COLOR_H
