/* date = October 5th 2024 3:01 pm */

#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

#define CODE(a,b,c,d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

#pragma pack(push, 1)

struct Hha_header {
#define MAGIC_VALUE CODE('h','h','a','f');
  u32 magic_value;
  
#define VERSION 0
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

struct Hha_sound {
  u32 first_asset_index;
  u32 sample_count;
  u32 next_id_to_play;
};

struct Hha_asset {
  u64 data_offset;
  u32 first_tag_index;
  u32 one_past_last_tag_index;
  
  union {
    Hha_bitmap bitmap;
    Hha_sound  sound;
  };
};
#pragma pack(pop)

#endif //FILE_FORMATS_H
