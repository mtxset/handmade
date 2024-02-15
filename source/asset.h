/* date = February 12th 2024 1:52 pm */

#ifndef ASSET_H
#define ASSET_H

struct Hero_bitmaps {
  Loaded_bmp head;
  Loaded_bmp cape;
  Loaded_bmp torso;
};

enum Asset_state {
  Asset_state_unloaded,
  Asset_state_queued,
  Asset_state_loaded,
  Asset_state_locked
};

struct Asset_slot {
  Asset_state state;
  Loaded_bmp *bitmap;
};

enum Asset_tag_id {
  Tag_smoothness,
  Tag_flatness,
  
  Tag_count,
};

struct Asset_tag {
  u32 id;
  f32 value;
};

enum Asset_type_id {
  Asset_null,
  
  Asset_shadow,
  Asset_tree,
  Asset_sword,
  //Asset_stairwell,
  Asset_rock,
  
  Asset_grass,
  Asset_tuft,
  Asset_stone,
  
  //Asset_monster,
  //Asset_familiar,
  
  Asset_count
};

struct Asset_type {
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

struct Asset {
  u32 first_tag_index;
  u32 one_past_last_tag_index;
  u32 slot_id;
};

struct Asset_bitmap_info {
  char *filename;
  v2 align_pcent;
};

struct Game_asset_list {
  
  struct Transient_state *tran_state;
  Memory_arena arena;
  
  u32 bitmap_count;
  Asset_bitmap_info *bitmap_info_list;
  Asset_slot *bitmap_list;
  
  u32 sound_count;
  Asset_slot *sound_list;
  
  u32 tag_count;
  Asset_tag *tag_list;
  
  u32 asset_count;
  Asset *asset_list;
  
  Hero_bitmaps hero_bitmaps[4];
  
  Asset_type asset_type_list[Asset_count];
  
  u32 debug_used_bitmap_count;
  u32 debug_used_asset_count;
  Asset_type *debug_asset_type;
};


struct Bitmap_id {
  u32 value;
};

struct Audio_id {
  u32 value;
};

internal void load_bitmap(Game_asset_list *asset_list, Bitmap_id id);
internal void load_sound(Game_asset_list *asset_list, Audio_id id);

#endif //ASSET_H
