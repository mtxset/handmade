/* date = September 22nd 2021 11:02 am */

#ifndef COLOR_H
#define COLOR_H

struct color_f32 {
    f32 r;
    f32 g;
    f32 b;
};

static const u32 color_black_byte  = 0xf0000000;
static const u32 color_white_byte  = 0xffffffff;
static const u32 color_purple_byte = 0x00ff00ff;
static const u32 color_cyan_byte   = 0x0000ffff;
static const u32 color_red_byte    = 0x00ff0000;
static const u32 color_green_byte  = 0x0000ff00;
static const u32 color_blue_byte   = 0x000000ff;
static const u32 color_gold_byte   = 0x00b48c06;
static const u32 color_yellow_byte = 0x00ffff00;
static const u32 color_gray_byte   = 0x0075787b;

static const color_f32 COLOR_GOLD = { 1.0f, 0.8f, .0f };

#endif //COLOR_H
