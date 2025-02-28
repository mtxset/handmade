/* date = October 5th 2024 3:01 pm */

#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

#define CODE(a,b,c,d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

#pragma pack(push, 1)

struct Bitmap_id { u32 value; };
struct Sound_id  { u32 value; };
struct Font_id   { u32 value; };

struct Hha_header {
#define HHA_MAGIC_VALUE CODE('h','h','a','f')
  u32 magic_value;
  
#define HHA_VERSION 0
  u32 version;
  
  u32 tag_count;
  u32 asset_type_count;
  u32 asset_count;
  
  u64 tag_list;
  u64 asset_type_list;
  u64 asset_list;
};

struct Hha_tag {
  u32 id;
  f32 value;
};

struct Hha_asset_type {
  u32 type_id;
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

struct Hha_bitmap {
  u32 dim[2];
  f32 align_pcent[2];
};

enum Hha_sound_chain {
  Hha_sound_chain_none,
  Hha_sound_chain_loop,
  Hha_sound_chain_advance,
};

struct Hha_sound {
  u32 sample_count;
  u32 channel_count;
  u32 chain;
};

struct Hha_font {
  u32 code_point_count;
  f32 ascender_height;
  f32 descender_height;
  f32 external_leading;
};

struct Hha_asset {
  u64 data_offset;
  u32 first_tag_index;
  u32 one_past_last_tag_index;
  
  union {
    Hha_bitmap bitmap;
    Hha_sound  sound;
    Hha_font   font;
  };
};
#pragma pack(pop)

#endif //FILE_FORMATS_H
