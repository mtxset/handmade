#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "platform.h"
#include "asset_type_id.h"
#include "file_formats.h"

#define _(x)

FILE *out = 0;

#define assert(expr) if (!(expr)) {*(i32*)0 = 0;}

struct Bitmap_id {
  u32 value;
};

struct Sound_id {
  u32 value;
};

struct Asset_bitmap_info {
  char *filename;
  f32 align_pcent[2];
};

struct Asset_sound_info {
  char *filename;
  u32 first_sample_index;
  u32 sample_count;
  Sound_id next_id_to_play;
};

struct Asset_tag {
  u32 id;
  f32 value;
};

struct Asset_type {
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

#define MAX_COUNT 1024

struct Asset {
  u64 data_offset;
  u32 first_tag_index;
  u32 one_past_last_tag_index;
  
  union {
    Asset_bitmap_info bitmap;
    Asset_sound_info  sound;
  };
};

struct Game_asset_list {
  u32 tag_count;
  Hha_tag tag_list[MAX_COUNT];
  
  u32 asset_type_count;
  Hha_asset_type asset_type_list[Asset_count];
  
  u32 asset_count;
  Asset asset_list[MAX_COUNT];
  
  Hha_asset_type *debug_asset_type;
  Asset *debug_asset;
};

static
void
begin_asset_type(Game_asset_list *asset_list, Asset_type_id type_id) {
  assert(asset_list->debug_asset_type == 0);
  
  asset_list->debug_asset_type = asset_list->asset_type_list + type_id;
  asset_list->debug_asset_type->type_id = type_id;
  asset_list->debug_asset_type->first_asset_index = asset_list->asset_count;
  asset_list->debug_asset_type->one_past_last_asset_index = asset_list->debug_asset_type->first_asset_index;
}

static
void
end_asset_type(Game_asset_list *asset_list) {
  assert(asset_list->debug_asset_type);
  
  asset_list->asset_count = asset_list->debug_asset_type->one_past_last_asset_index;
  asset_list->debug_asset_type = 0;
  asset_list->debug_asset = 0;
}

static
Bitmap_id
add_bitmap_asset(Game_asset_list* asset_list, char *filename, f32 align_pcent_x = .5f, f32 align_pcent_y = .5f) {
  assert(asset_list->debug_asset_type);
  assert(asset_list->debug_asset_type->one_past_last_asset_index < array_count(asset_list->asset_list));
  
  Bitmap_id id = {asset_list->debug_asset_type->one_past_last_asset_index++};
  
  Asset *asset = asset_list->asset_list + id.value;
  
  asset->first_tag_index = asset_list->tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->bitmap.filename = filename;
  asset->bitmap.align_pcent[0] = align_pcent_x;
  asset->bitmap.align_pcent[1] = align_pcent_y;
  
  asset_list->debug_asset = asset;
  
  return id;
}

static
Sound_id
add_sound_asset(Game_asset_list* asset_list, char *filename, u32 first_sample_index = 0, u32 sample_count = 0) {
  assert(asset_list->debug_asset_type);
  assert(asset_list->debug_asset_type->one_past_last_asset_index < array_count(asset_list->asset_list));
  
  Sound_id id = {asset_list->debug_asset_type->one_past_last_asset_index++};
  
  Asset *asset = asset_list->asset_list + id.value;
  
  asset->first_tag_index = asset_list->tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->sound.filename = filename;
  asset->sound.first_sample_index = first_sample_index;
  asset->sound.sample_count = sample_count;
  asset->sound.next_id_to_play.value = 0;
  
  asset_list->debug_asset = asset;
  
  return id;
}

static
void
add_tag(Game_asset_list *asset_list, Asset_tag_id id, f32 value) {
  assert(asset_list->debug_asset);
  
  
  asset_list->debug_asset->one_past_last_tag_index++;
  Hha_tag *tag = asset_list->tag_list + asset_list->tag_count++;
  
  tag->id = id;
  tag->value = value;
}

int
main(int arg_count, char **arg_list) {
  
  Game_asset_list _asset_list_;
  Game_asset_list *asset_list = &_asset_list_;
  
  asset_list->tag_count = 1;
  asset_list->asset_count = 1;
  asset_list->debug_asset_type = 0;
  asset_list->debug_asset = 0;
  
  begin_asset_type(asset_list, Asset_sword);
  add_bitmap_asset(asset_list, "../data/sword.bmp");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_grass);
  add_bitmap_asset(asset_list, "../data/grass00.bmp");
  add_bitmap_asset(asset_list, "../data/grass01.bmp");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_tuft);
  add_bitmap_asset(asset_list, "../data/tuft00.bmp");
  add_bitmap_asset(asset_list, "../data/tuft01.bmp");
  add_bitmap_asset(asset_list, "../data/tuft02.bmp");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_stone);
  add_bitmap_asset(asset_list, "../data/ground00.bmp");
  add_bitmap_asset(asset_list, "../data/ground01.bmp");
  add_bitmap_asset(asset_list, "../data/ground02.bmp");
  add_bitmap_asset(asset_list, "../data/ground03.bmp");
  end_asset_type(asset_list);
  
  f32 angle_right = 0.0f  * TAU;
  f32 angle_back  = 0.25f * TAU;
  f32 angle_left  = 0.5f  * TAU;
  f32 angle_front = 0.75f * TAU;
  
  f32 hero_align[] = {0.5f, 0.156682029f};
  
  begin_asset_type(asset_list, Asset_head);
  add_bitmap_asset(asset_list, "../data/test_hero_right_head.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_head.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_head.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_head.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_front);
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_cape);
  add_bitmap_asset(asset_list, "../data/test_hero_right_cape.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_cape.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_cape.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_cape.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_front);
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_torso);
  add_bitmap_asset(asset_list, "../data/test_hero_right_torso.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_torso.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_torso.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_torso.bmp", hero_align[0], hero_align[1]);
  add_tag(asset_list, Tag_facing_dir, angle_front);
  end_asset_type(asset_list);
  
  ////////// Sounds
  
  begin_asset_type(asset_list, Asset_bloop);
  add_sound_asset(asset_list, "../data/sounds/bloop_00.wav");
  add_sound_asset(asset_list, "../data/sounds/bloop_01.wav");
  add_sound_asset(asset_list, "../data/sounds/bloop_02.wav");
  add_sound_asset(asset_list, "../data/sounds/bloop_03.wav");
  add_sound_asset(asset_list, "../data/sounds/bloop_04.wav");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_crack);
  add_sound_asset(asset_list, "../data/sounds/crack_00.wav");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_drop);
  add_sound_asset(asset_list, "../data/sounds/drop_00.wav");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_glide);
  add_sound_asset(asset_list, "../data/sounds/glide_00.wav");
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_puhp);
  add_sound_asset(asset_list, "../data/sounds/puhp_00.wav");
  add_sound_asset(asset_list, "../data/sounds/puhp_01.wav");
  end_asset_type(asset_list);
  
  errno_t open_result = fopen_s(&out, "test.hha", "wb");
  
  if (open_result == 0) {
    
    Hha_header header = {};
    
    header.magic_value = MAGIC_VALUE;
    header.version = VERSION;
    header.tag_count = asset_list->tag_count;
    header.asset_type_count = Asset_count;
    header.asset_count = asset_list->asset_count;
    
    u32 tag_array_size = header.tag_count * sizeof(Hha_tag);
    u32 asset_type_array_size = header.asset_type_count * sizeof(Hha_asset_type);
    u32 asset_array_size = header.asset_count * sizeof(Hha_asset);
    
    header.tag_list = sizeof(Hha_header);
    header.asset_type_list = header.tag_list + tag_array_size;
    header.asset_list = header.asset_type_list + asset_type_array_size;
    
    fwrite(&header, sizeof(Hha_header), _(count)1, out);
    fwrite(asset_list->tag_list, tag_array_size, _(count)1, out);
    fwrite(asset_list->asset_type_list, asset_type_array_size, _(count)1, out);
    
    fclose(out);
  }
  else {
    printf("can't open file\n");
  }
  
  return 0;
}