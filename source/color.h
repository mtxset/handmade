/* date = September 22nd 2021 11:02 am */

// https://pemavirtualhub.wordpress.com/2016/06/20/opengl-color-codes/

#ifndef COLOR_H
#define COLOR_H

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

global_var const v3 red_v3    = { 1.0f, 0.0f, 0.0f };
global_var const v3 green_v3  = { 0.0f, 1.0f, 0.0f };
global_var const v3 blue_v3   = { 0.0f, 0.0f, 1.0f };
global_var const v3 gold_v3   = { 1.0f, 0.8f, 0.0f };
global_var const v3 white_v3  = { 1.0f, 1.0f, 1.0f };
global_var const v3 yellow_v3 = { 1.0f, 1.0f, 0.0f };
global_var const v3 grey_v3   = { 0.1f, 0.1f, 0.1f };

inline
v3 color_v3_v4(v4 color) {
    v3 result;
    
    result.r = color.r;
    result.g = color.g;
    result.b = color.b;
    
    return result;
}

#endif //COLOR_H
