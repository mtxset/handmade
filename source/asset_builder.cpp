#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "asset_type_id.h"

FILE *out = 0;

#define assert(expr) if (!(expr)) {*(i32*)0 = 0;}

struct Asset_bitmap_info {
  char *filename;
  f32 align_pcent[2];
};

struct Asset_sound_info {
  char *filename;
  u32 first_sample_index;
  u32 sample_count;
  u32 next_id_to_play;
};

struct Asset_tag {
  u32 id;
  f32 value;
};

struct Asset_type {
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

struct Asset {
  u64 data_offset;
  u32 first_tag_index;
  u32 one_past_last_tag_index;
};

Asset_bitmap_info bitmap_info_list[4096]; u32 bitmap_count;
Asset_sound_info  sound_info_list[4096]; u32 sound_count;
Asset_tag         tag_list[4096]; u32 tag_count;
Asset             asset_list[4096]; u32 asset_count;
Asset_type        asset_type_list[Asset_count];

u32 debug_used_bitmap_count;
u32 debug_used_sound_count;
u32 debug_used_asset_count;
u32 debug_used_tag_count;
Asset_type *debug_asset_type;
Asset *debug_asset;

static
void
begin_asset_type(Asset_type_id type_id) {
  assert(debug_asset_type == 0);
  
  debug_asset_type = asset_type_list + type_id;
  debug_asset_type->first_asset_index = debug_used_asset_count;
  debug_asset_type->one_past_last_asset_index = debug_asset_type->first_asset_index;
}

static
void
end_asset_type(Game_asset_list *asset_list) {
  assert(debug_asset_type);
  
  debug_used_asset_count = debug_asset_type->one_past_last_asset_index;
  debug_asset_type = 0;
  debug_asset = 0;
}

static
Bitmap_id
debug_add_bitmap_info(Game_asset_list* asset_list, char *filename, v2 align_pcent) {
  assert(debug_used_bitmap_count < bitmap_count);
  
  Bitmap_id id = {debug_used_bitmap_count++};
  
  Asset_bitmap_info *info = bitmap_info_list + id.value;
  info->filename = mem_push_string(&arena, filename);
  //info->filename = filename;
  info->align_pcent = align_pcent;
  
  return id;
}


static
Sound_id
debug_add_sound_info(Game_asset_list* asset_list, char *filename, u32 first_sample_index, u32 sample_count) {
  assert(debug_used_sound_count < sound_count);
  
  Sound_id id = {debug_used_sound_count++};
  
  Asset_sound_info *info = sound_info_list + id.value;
  info->filename = mem_push_string(&arena, filename);
  //info->filename = filename;
  info->first_sample_index = first_sample_index;
  info->sample_count = sample_count;
  info->next_id_to_play.value = 0;
  
  return id;
}

static
void
add_bitmap_asset(char *filename, v2 align_pcent = {0.5, 0.5}) {
  assert(debug_asset_type);
  
  Asset *asset = asset_list + debug_asset_type->one_past_last_asset_index++;
  asset->first_tag_index = debug_used_tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->slot_id = debug_add_bitmap_info(asset_list, filename, align_pcent).value;
  
  debug_asset = asset;
}

static
Asset*
add_sound_asset(char *filename, u32 first_sample_index = 0, u32 sample_count = 0) {
  assert(debug_asset_type);
  assert(debug_asset_type->one_past_last_asset_index < asset_count);
  
  Asset *asset = asset_list + debug_asset_type->one_past_last_asset_index++;
  asset->first_tag_index = debug_used_tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->slot_id = debug_add_sound_info(asset_list, filename, first_sample_index, sample_count).value;
  
  debug_asset = asset;
  
  return asset;
}


static
void
add_tag(Asset_tag_id id, f32 value) {
  assert(debug_asset);
  
  debug_asset->one_past_last_tag_index++;
  Asset_tag *tag = tag_list + debug_used_tag_count++;
  
  tag->id = id;
  tag->value = value;
}



int
main() {
  
  u32 result = fopen_s(&out, "asset.file", "wb");
  
  assert(out == 0);
  
  fclose(out);
  
  
#if 0
  begin_asset_type(asset_list, Asset_sword);
  //add_bitmap_asset(asset_list, "../data/rock03.bmp", v2{0.5f, 0.65625f});
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
  
  v2 hero_align = {0.5f, 0.156682029f};
  
  begin_asset_type(asset_list, Asset_head);
  add_bitmap_asset(asset_list, "../data/test_hero_right_head.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_head.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_head.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_head.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_front);
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_cape);
  add_bitmap_asset(asset_list, "../data/test_hero_right_cape.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_cape.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_cape.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_cape.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_front);
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_torso);
  add_bitmap_asset(asset_list, "../data/test_hero_right_torso.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_right);
  add_bitmap_asset(asset_list, "../data/test_hero_back_torso.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_back);
  add_bitmap_asset(asset_list, "../data/test_hero_left_torso.bmp", hero_align);
  add_tag(asset_list, Tag_facing_dir, angle_left);
  add_bitmap_asset(asset_list, "../data/test_hero_front_torso.bmp", hero_align);
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
  
  {
    u32 music_chunk = 48000 * 10;
    u32 total_music_sample_count = 7468095;
    begin_asset_type(asset_list, Asset_music);
    Asset *last_piece = 0;
    char *path_to_music = "../data/sounds/music_test.wav";
    for (u32 first_sample_index = 0; first_sample_index < total_music_sample_count; first_sample_index += music_chunk) {
      
      u32 sample_count = total_music_sample_count - first_sample_index;
      
      if (sample_count > music_chunk) {
        sample_count = music_chunk;
      }
      
      Asset *this_piece = add_sound_asset(asset_list, path_to_music, first_sample_index, sample_count);
      
      if (last_piece) {
        sound_info_list[last_piece->slot_id].next_id_to_play.value = this_piece->slot_id;
      }
      last_piece = this_piece;
    }
    
    end_asset_type(asset_list);
  }
  
  begin_asset_type(asset_list, Asset_puhp);
  add_sound_asset(asset_list, "../data/sounds/puhp_00.wav");
  add_sound_asset(asset_list, "../data/sounds/puhp_01.wav");
  end_asset_type(asset_list);
  
#endif
  return 0;
}