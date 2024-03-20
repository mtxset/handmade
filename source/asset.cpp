

internal
v2
top_down_align(Loaded_bmp* bitmap, v2 align) {
  align.y = (f32)(bitmap->height - 1) - align.y;
  
  align.x = safe_ratio_0(align.x, (f32)bitmap->width);
  align.y = safe_ratio_0(align.y, (f32)bitmap->height);
  
  return align;
}

internal
void
set_top_down_align(Hero_bitmaps *bitmap, v2 align) {
  align = top_down_align(&bitmap->head, align);
  
  bitmap->head.align_pcent = align;
  bitmap->cape.align_pcent = align;
  bitmap->torso.align_pcent = align;
}

internal
Loaded_bmp
debug_load_bmp(char* file_name, v2 align_pcent = v2{.5, .5}) {
  Loaded_bmp result = {};
  
  Debug_file_read_result file_result = debug_read_entire_file(file_name);
  
  macro_assert(file_result.bytes_read != 0);
  
  Bitmap_header* header = (Bitmap_header*)file_result.content;
  
  macro_assert(result.height >= 0);
  macro_assert(header->Compression == 3);
  
  result.memory = (u32*)((u8*)file_result.content + header->BitmapOffset);
  
  result.width  = header->Width;
  result.height = header->Height;
  result.align_pcent = align_pcent;
  result.width_over_height = safe_ratio_0((f32)result.width, (f32)result.height);
  f32 pixel_to_meter = 1.0f / 42.0f;
  result.native_height = pixel_to_meter * result.height;
  
  u32 mask_red   = header->RedMask;
  u32 mask_green = header->GreenMask;
  u32 mask_blue  = header->BlueMask;
  u32 mask_alpha = ~(mask_red | mask_green | mask_blue);
  
  Bit_scan_result scan_red   = find_least_significant_first_bit(mask_red);
  Bit_scan_result scan_green = find_least_significant_first_bit(mask_green);
  Bit_scan_result scan_blue  = find_least_significant_first_bit(mask_blue);
  Bit_scan_result scan_alpha = find_least_significant_first_bit(mask_alpha);
  
  macro_assert(scan_red.found);
  macro_assert(scan_green.found);
  macro_assert(scan_blue.found);
  macro_assert(scan_alpha.found);
  
  i32 shift_alpha_down = (i32)scan_alpha.index;
  i32 shift_red_down   = (i32)scan_red.index;
  i32 shift_green_down = (i32)scan_green.index;
  i32 shift_blue_down  = (i32)scan_blue.index;
  
  u32* source_dest = (u32*)result.memory;
  for (i32 y = 0; y < header->Height; y++) {
    for (i32 x = 0; x < header->Width; x++) {
      u32 color = *source_dest;
      
      v4 texel = {
        (f32)((color & mask_red)   >> shift_red_down),
        (f32)((color & mask_green) >> shift_green_down),
        (f32)((color & mask_blue)  >> shift_blue_down),
        (f32)((color & mask_alpha) >> shift_alpha_down),
      };
      
      texel = srgb255_to_linear1(texel);
      
      bool premultiply_alpha = true;
      if (premultiply_alpha) {
        texel.rgb *= texel.a;
      }
      
      texel = linear1_to_srgb255(texel);
      
      *source_dest++ = (((u32)(texel.a + 0.5f) << 24) | 
                        ((u32)(texel.r + 0.5f) << 16) | 
                        ((u32)(texel.g + 0.5f) << 8 ) | 
                        ((u32)(texel.b + 0.5f) << 0 ));
    }
  }
  
  i32 bytes_per_pixel = BITMAP_BYTES_PER_PIXEL;
  // top-to-bottom
#if 0
  result.pitch = -result.width * bytes_per_pixel;
  result.memory = (u8*)result.memory - result.pitch * (result.height - 1);
#else
  // bottom-to-top
  result.pitch = result.width * bytes_per_pixel;
#endif
  
  return result;
}

struct Load_bitmap_work {
  Game_asset_list *asset_list;
  Bitmap_id id;
  Task_with_memory *task;
  Loaded_bmp *bitmap;
  
  Asset_state final_state;
};

internal 
PLATFORM_WORK_QUEUE_CALLBACK(load_bitmap_work) {
  Load_bitmap_work *work = (Load_bitmap_work*)data;
  
  Asset_bitmap_info *info = work->asset_list->bitmap_info_list + work->id.value;
  *work->bitmap = debug_load_bmp(info->filename, info->align_pcent);
  
  _WriteBarrier();
  
  work->asset_list->bitmap_list[work->id.value].bitmap = work->bitmap;
  work->asset_list->bitmap_list[work->id.value].state = work->final_state;
  
  end_task_with_mem(work->task);
}

internal
void 
load_bitmap(Game_asset_list *asset_list, Bitmap_id id) {
  
  if (id.value &&
      atomic_compare_exchange_u32((u32*)&asset_list->bitmap_list[id.value].state, Asset_state_unloaded, Asset_state_queued) != Asset_state_unloaded) 
    return;
  
  Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
  
  if (!task)
    return;
  
  Load_bitmap_work *work = mem_push_struct(&task->arena, Load_bitmap_work);
  
  work->asset_list = asset_list;
  work->id = id;
  work->task = task;
  work->bitmap = mem_push_struct(&asset_list->arena, Loaded_bmp);
  work->final_state = Asset_state_loaded;
  
  platform_add_entry(asset_list->tran_state->low_priority_queue, load_bitmap_work, work);
}

struct Riff_iterator {
  u8 *at;
  u8 *stop;
};

inline
Riff_iterator
parse_chunk_at(void *at, void *stop) {
  Riff_iterator iterator;
  
  iterator.at = (u8*)at;
  iterator.stop = (u8*)stop;
  
  return iterator;
}

inline 
Riff_iterator
next_chunk(Riff_iterator iter) {
  Wave_chunk *chunk = (Wave_chunk*)iter.at;
  u32 size = (chunk->size + 1) & ~1;
  iter.at += sizeof(Wave_chunk) + size;
  
  return iter;
}

inline
void*
get_chunk_data(Riff_iterator iter) {
  void *result = iter.at + sizeof(Wave_chunk);
  
  return result;
}

inline
u32
get_type(Riff_iterator iter) {
  Wave_chunk *chunk = (Wave_chunk*)iter.at;
  u32 result = chunk->id;
  
  return result;
}

inline
u32
get_chunk_data_size(Riff_iterator iter) {
  Wave_chunk *chunk = (Wave_chunk*)iter.at;
  u32 result = chunk->size;
  
  return result;
}

internal
Loaded_sound
debug_load_wav(char *filename) {
  Loaded_sound result = {};
  
  Debug_file_read_result read_result = debug_read_entire_file(filename);
  macro_assert(read_result.content);
  
  Wave_header *header = (Wave_header*)read_result.content;
  macro_assert(header->riffid == Wave_ChunkID_RIFF);
  macro_assert(header->waveid == Wave_ChunkID_WAVE);
  
  u32 channel_count = 0;
  u32 sample_data_size = 0;
  i16 *sample_data = 0;
  
  for (Riff_iterator iter = parse_chunk_at(header + 1, (u8*)(header + 1) + header->size - 4);
       iter.at < iter.stop;
       iter = next_chunk(iter)) {
    
    switch (get_type(iter)) {
      
      case Wave_ChunkID_fmt: {
        Wave_fmt *fmt = (Wave_fmt*)get_chunk_data(iter);
        macro_assert(fmt->format_tag == 1);
        macro_assert(fmt->samples_per_sec == 48000);
        macro_assert(fmt->bits_per_sample == 16);
        macro_assert(fmt->block_align == sizeof(i16) * fmt->channels);
        channel_count = fmt->channels;
        u32 u = 0;
      } break;
      
      case Wave_ChunkID_data: {
        sample_data = (i16*)get_chunk_data(iter);
        sample_data_size = get_chunk_data_size(iter);
      } break;
      
      //default: macro_assert(!"FAILURE");
    }
    
  }
  
  macro_assert(channel_count && sample_data_size && sample_data);
  
  result.channel_count = channel_count;
  result.sample_count = sample_data_size / (channel_count * sizeof(i16));
  
  macro_assert(channel_count == 2);
  
  result.samples[0] = sample_data;
  result.samples[1] = sample_data + result.sample_count;
  
  for (u32 sample_index = 0; sample_index < result.sample_count; sample_index++) {
    i16 source = sample_data[2 * sample_index];
    
    sample_data[2 * sample_index] = sample_data[sample_index];
    sample_data[sample_index] = source;
  }
  
  return result;
}

struct Load_sound_work {
  Game_asset_list *asset_list;
  Sound_id id;
  Task_with_memory *task;
  Loaded_sound *sound;
  
  Asset_state final_state;
};

internal 
PLATFORM_WORK_QUEUE_CALLBACK(load_sound_work) {
  Load_sound_work *work = (Load_sound_work*)data;
  
  Asset_sound_info *info = work->asset_list->bitmap_sound_list + work->id.value;
  *work->sound = debug_load_wav(info->filename);
  
  _WriteBarrier();
  
  work->asset_list->sound_list[work->id.value].sound = work->sound;
  work->asset_list->sound_list[work->id.value].state = work->final_state;
  
  end_task_with_mem(work->task);
}

internal void
load_sound(Game_asset_list *asset_list, Sound_id id) {
  if (id.value &&
      atomic_compare_exchange_u32((u32*)&asset_list->sound_list[id.value].state, Asset_state_unloaded, Asset_state_queued) != Asset_state_unloaded) 
    return;
  
  Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
  
  if (!task)
    return;
  
  Load_sound_work *work = mem_push_struct(&task->arena, Load_sound_work);
  
  work->asset_list = asset_list;
  work->id = id;
  work->task = task;
  work->sound = mem_push_struct(&asset_list->arena, Loaded_sound);
  work->final_state = Asset_state_loaded;
  
  platform_add_entry(asset_list->tran_state->low_priority_queue, load_sound_work, work);
}

internal
u32
get_best_match_asset_from(Game_asset_list *asset_list, Asset_type_id type_id, Asset_vector *match_vector, Asset_vector *weight_vector) {
  
  u32 result = {};
  
  f32 best_diff = FLT_MAX;
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  for (u32 asset_index = type->first_asset_index; asset_index < type->one_past_last_asset_index; asset_index++) {
    Asset *asset = asset_list->asset_list + asset_index;
    
    f32 total_weight_diff = 0.0f;
    for (u32 tag_index = asset->first_tag_index; tag_index < asset->one_past_last_tag_index; tag_index++) {
      Asset_tag *tag = asset_list->tag_list + tag_index;
      
      f32 a = match_vector->e[tag->id];
      f32 b = tag->value;
      f32 d0 = absolute(a - b);
      f32 d1 = absolute((a - asset_list->tag_range[tag->id] * sign_of(a)) - b);
      f32 diff = min(d0, d1);
      
      f32 weighted = weight_vector->e[tag->id] * diff;
      total_weight_diff += weighted;
    }
    
    if (best_diff > total_weight_diff) {
      best_diff = total_weight_diff;
      result = asset->slot_id;
    }
  }
  
  return result;
}

internal
u32
get_random_slot_from(Game_asset_list *asset_list, Asset_type_id type_id, Random_series *series) {
  u32 result = {};
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  if (type->first_asset_index != type->one_past_last_asset_index) {
    u32 count = type->one_past_last_asset_index - type->first_asset_index;
    u32 choice = random_choise(series, count);
    
    Asset *asset = asset_list->asset_list + type->first_asset_index + choice;
    result = asset->slot_id;
  }
  
  return result;
}

internal
u32
get_first_slot_from(Game_asset_list* asset_list, Asset_type_id type_id) {
  
  u32 result = 0;
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  if (type->first_asset_index != type->one_past_last_asset_index) {
    Asset *asset = asset_list->asset_list + type->first_asset_index;
    result = asset->slot_id;
  }
  
  return result;
  
}

inline
Bitmap_id
get_best_match_bitmap_from(Game_asset_list* asset_list, Asset_type_id type_id, Asset_vector *match_vector, Asset_vector* weight_vector) {
  Bitmap_id result = {};
  result.value = get_best_match_asset_from(asset_list, type_id, match_vector, weight_vector);
  
  return result;
}

inline
Bitmap_id
get_first_bitmap_from(Game_asset_list *asset_list, Asset_type_id type_id) {
  Bitmap_id result = {};
  result.value = get_first_slot_from(asset_list, type_id);
  
  return result;
}

inline
Bitmap_id
get_random_bitmap_from(Game_asset_list *asset_list, Asset_type_id type_id, Random_series *series) {
  Bitmap_id result = {};
  result.value = get_random_slot_from(asset_list, type_id, series);
  
  return result;
}


inline
Sound_id
get_best_match_sound_from(Game_asset_list* asset_list, Asset_type_id type_id, Asset_vector *match_vector, Asset_vector* weight_vector) {
  Sound_id result = {};
  result.value = get_best_match_asset_from(asset_list, type_id, match_vector, weight_vector);
  
  return result;
}

inline
Sound_id
get_first_sound_from(Game_asset_list *asset_list, Asset_type_id type_id) {
  Sound_id result = {};
  result.value = get_first_slot_from(asset_list, type_id);
  
  return result;
}

inline
Sound_id
get_random_sound_from(Game_asset_list *asset_list, Asset_type_id type_id, Random_series *series) {
  Sound_id result = {};
  result.value = get_random_slot_from(asset_list, type_id, series);
  
  return result;
}

internal
void
begin_asset_type(Game_asset_list *asset_list, Asset_type_id type_id) {
  macro_assert(asset_list->debug_asset_type == 0);
  
  asset_list->debug_asset_type = asset_list->asset_type_list + type_id;
  asset_list->debug_asset_type->first_asset_index = asset_list->debug_used_asset_count;
  asset_list->debug_asset_type->one_past_last_asset_index = asset_list->debug_asset_type->first_asset_index;
}

internal
void
end_asset_type(Game_asset_list *asset_list) {
  macro_assert(asset_list->debug_asset_type);
  
  asset_list->debug_used_asset_count = asset_list->debug_asset_type->one_past_last_asset_index;
  asset_list->debug_asset_type = 0;
  asset_list->debug_asset = 0;
}

internal
Bitmap_id
debug_add_bitmap_info(Game_asset_list* asset_list, char *filename, v2 align_pcent) {
  macro_assert(asset_list->debug_used_bitmap_count < asset_list->bitmap_count);
  
  Bitmap_id id = {asset_list->debug_used_bitmap_count++};
  
  Asset_bitmap_info *info = asset_list->bitmap_info_list + id.value;
  info->filename = filename;
  info->align_pcent = align_pcent;
  
  return id;
}

internal
void
add_bitmap_asset(Game_asset_list *asset_list, char *filename, v2 align_pcent = {0.5, 0.5}) {
  macro_assert(asset_list->debug_asset_type);
  
  Asset *asset = asset_list->asset_list + asset_list->debug_asset_type->one_past_last_asset_index++;
  asset->first_tag_index = asset_list->debug_used_tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->slot_id = debug_add_bitmap_info(asset_list, filename, align_pcent).value;
  
  asset_list->debug_asset = asset;
}

internal
void
add_tag(Game_asset_list *asset_list, Asset_tag_id id, f32 value) {
  macro_assert(asset_list->debug_asset);
  
  asset_list->debug_asset->one_past_last_tag_index++;
  Asset_tag *tag = asset_list->tag_list + asset_list->debug_used_tag_count++;
  
  tag->id = id;
  tag->value = value;
}

internal
Game_asset_list*
allocate_game_asset_list(Memory_arena *arena, size_t size, Transient_state *tran_state) {
  
  Game_asset_list *asset_list = mem_push_struct(arena, Game_asset_list);
  sub_arena(&asset_list->arena, arena, size);
  asset_list->tran_state = tran_state;
  
  for (u32 tag_type = 0; tag_type < Tag_count; tag_type++) {
    asset_list->tag_range[tag_type] = 1000000.0f;
  }
  asset_list->tag_range[Tag_facing_dir] = TAU;
  
  asset_list->bitmap_count = 256 * Asset_count;
  asset_list->bitmap_info_list = mem_push_array(arena, asset_list->bitmap_count, Asset_bitmap_info);
  asset_list->bitmap_list = mem_push_array(arena, asset_list->bitmap_count, Asset_slot);
  
  asset_list->sound_count = 1;
  asset_list->sound_list = mem_push_array(arena, asset_list->sound_count, Asset_slot);
  
  asset_list->asset_count = asset_list->sound_count + asset_list->bitmap_count;
  asset_list->asset_list = mem_push_array(arena, asset_list->asset_count, Asset);
  
  asset_list->tag_count = 1024 * Asset_count;
  asset_list->tag_list = mem_push_array(arena, asset_list->tag_count, Asset_tag);
  
  asset_list->debug_used_bitmap_count = 1;
  asset_list->debug_used_asset_count = 1;
  
  begin_asset_type(asset_list, Asset_shadow);
  add_bitmap_asset(asset_list, "../data/test_hero_shadow.bmp", v2{0.5f, 0.156682029f});
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_tree);
  add_bitmap_asset(asset_list, "../data/tree.bmp", v2{0.434782594, 0.0169491526});
  end_asset_type(asset_list);
  
  /*
    george-front-0.bmp
      x;0.520833313;float;
    y;0.104166664;float;
    
    ship.bmp
      x;0.5;float;
    y;0.0394736826;float;
    */
  
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
  
  return asset_list;
}