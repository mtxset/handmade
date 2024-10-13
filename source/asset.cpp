struct Load_asset_work {
  Task_with_memory *task;
  Asset_slot *slot;
  
  Platform_file_handle *handle;
  
  u64 offset;
  u64 size;
  
  void *destination;
  
  Asset_state final_state;
};

internal 
PLATFORM_WORK_QUEUE_CALLBACK(load_asset_work) {
  Load_asset_work *work = (Load_asset_work*)data;
  
  platform.read_data_from_file(work->handle, work->offset, work->size, work->destination);
  
  _WriteBarrier();
  
  if (work->handle->no_errors) {
    work->slot->state = work->final_state;
  }
  
  end_task_with_mem(work->task);
}

inline 
Platform_file_handle*
get_file_handle_for(Game_asset_list *asset_list, u32 file_index)
{
  assert(file_index < asset_list->file_count);
  
  Platform_file_handle *result = asset_list->file_list[file_index].handle;
  
  return result;
}

void 
load_bitmap(Game_asset_list *asset_list, Bitmap_id id) {
  
  Asset_slot *slot = asset_list->slot_list + id.value;
  
  if (id.value &&
      atomic_compare_exchange_u32((u32*)&slot->state, Asset_state_queued, Asset_state_unloaded) == Asset_state_unloaded) {
    
    Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
    
    if (task) {
      
      Asset *asset = asset_list->asset_list + id.value;
      
      Hha_bitmap *info = &asset->hha.bitmap;
      Loaded_bmp *bitmap = &slot->bitmap;
      
      bitmap->align_pcent = {info->align_pcent[0], info->align_pcent[1]};
      bitmap->width_over_height = (f32)info->dim[0] / (f32)info->dim[1];
      bitmap->width = truncate_i32_u16(info->dim[0]);
      bitmap->height = truncate_i32_u16(info->dim[1]);
      bitmap->pitch = bitmap->width * _(bytes per pixel)4;
      
      u32 memory_size = bitmap->pitch * bitmap->height;
      bitmap->memory = mem_push_size(&asset_list->arena, memory_size);
      
      Load_asset_work *work = mem_push_struct(&task->arena, Load_asset_work);
      
      work->task = task;
      work->slot = asset_list->slot_list + id.value;
      work->handle = get_file_handle_for(asset_list, asset->file_index);
      work->offset = asset->hha.data_offset;
      work->size = memory_size;
      work->destination = bitmap->memory;
      work->final_state = Asset_state_loaded;
      
      platform.add_entry(asset_list->tran_state->low_priority_queue, load_asset_work, work);
    }
    else {
      slot->state = Asset_state_unloaded;
    }
    
  }
}


void
load_sound(Game_asset_list *asset_list, Sound_id id) {
  
  Asset_slot *slot = asset_list->slot_list + id.value;
  
  if (id.value &&
      (atomic_compare_exchange_u32((u32*)&asset_list->slot_list[id.value].state, Asset_state_queued, Asset_state_unloaded) == Asset_state_unloaded)) {
    
    Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
    
    if (task) {
      Asset *asset = asset_list->asset_list + id.value;
      
      Hha_sound *info = &asset->hha.sound;
      Loaded_sound *sound = &slot->sound;
      
      sound->sample_count = info->sample_count;
      sound->channel_count = info->channel_count;
      u32 channel_size = sound->sample_count * sizeof(i16);
      u32 memory_size  = sound->channel_count * channel_size;
      
      void *memory = mem_push_size(&asset_list->arena, memory_size);
      
      i16 *sound_at = (i16*)memory;
      for (u32 channel_index = 0; channel_index < sound->channel_count; channel_index++) {
        sound->samples[channel_index] = sound_at;
        sound_at += channel_size;
      }
      
      Load_asset_work *work = mem_push_struct(&task->arena, Load_asset_work);
      
      work->task = task;
      work->slot = asset_list->slot_list + id.value;
      work->handle = get_file_handle_for(asset_list, asset->file_index);
      work->offset = asset->hha.data_offset;
      work->size = memory_size;
      work->destination = memory;
      work->final_state = Asset_state_loaded;
      
      platform.add_entry(asset_list->tran_state->low_priority_queue, load_asset_work, work);
    }
    else {
      slot->state = Asset_state_unloaded;
    }
  }
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
    for (u32 tag_index = asset->hha.first_tag_index; tag_index < asset->hha.one_past_last_tag_index; tag_index++) {
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
      result = asset_index;
    }
  }
  
  return result;
}

internal
u32
get_random_slot_from(Game_asset_list *asset_list, Asset_type_id type_id, Random_series *series) {
  u32 result = 0;
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  if (type->first_asset_index != type->one_past_last_asset_index) {
    u32 count = type->one_past_last_asset_index - type->first_asset_index;
    u32 choice = random_choise(series, count);
    
    result = type->first_asset_index + choice;
  }
  
  return result;
}

internal
u32
get_first_slot_from(Game_asset_list* asset_list, Asset_type_id type_id) {
  
  u32 result = 0;
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  if (type->first_asset_index != type->one_past_last_asset_index) {
    result = type->first_asset_index;
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
Game_asset_list*
allocate_game_asset_list(Memory_arena *arena, size_t size, Transient_state *tran_state) {
  
  Game_asset_list *asset_list = mem_push_struct(arena, Game_asset_list);
  sub_arena(&asset_list->arena, arena, size);
  asset_list->tran_state = tran_state;
  
  for (u32 tag_type = 0; tag_type < Tag_count; tag_type++) {
    asset_list->tag_range[tag_type] = 1000000.0f;
  }
  asset_list->tag_range[Tag_facing_dir] = TAU;
  
  asset_list->tag_count = 1;
  asset_list->asset_count = 1;
  
  Platform_file_group *file_group = platform.get_all_files_of_type_begin("hha");
  asset_list->file_count = file_group->file_count;
  asset_list->file_list = mem_push_array(arena, asset_list->file_count, Asset_file);
  
  for (u32 file_index = 0; file_index < asset_list->file_count; file_index++) {
    Asset_file *file = asset_list->file_list + file_index;
    
    file->tag_base = asset_list->tag_count;
    
    mem_zero_struct(file->header);
    file->handle = platform.open_file(file_group, file_index);
    platform.read_data_from_file(file->handle, 0, sizeof(file->header), &file->header);
    
    u32 asset_type_array_size = file->header.asset_type_count * sizeof(Hha_asset_type);
    file->asset_type_array = (Hha_asset_type*)mem_push_size(arena, asset_type_array_size);
    platform.read_data_from_file(file->handle, file->header.asset_type_list, asset_type_array_size, file->asset_type_array);
    
    assert(file->handle->no_errors);
    
    if (file->header.magic_value != HHA_MAGIC_VALUE) {
      platform.file_error(file->handle, "wrong magic value");
    }
    
    if (file->header.version > HHA_VERSION) {
      platform.file_error(file->handle, "unsupperted version");
    }
    
    // skip first cuz it's null
    asset_list->tag_count   += (file->header.tag_count - 1);
    asset_list->asset_count += (file->header.asset_count - 1);
  }
  platform.get_all_files_of_type_end(file_group);
  
  asset_list->asset_list = mem_push_array(arena, asset_list->asset_count, Asset);
  asset_list->slot_list  = mem_push_array(arena, asset_list->asset_count, Asset_slot);
  asset_list->tag_list   = mem_push_array(arena, asset_list->tag_count, Asset_tag);
  
  mem_zero_struct(asset_list->tag_list[0]);
  
  for (u32 file_index = 0; file_index < asset_list->file_count; file_index++) {
    Asset_file *file = asset_list->file_list + file_index;
    
    if (file->handle->no_errors) {
      u32 tag_array_size = sizeof(Hha_tag) * (file->header.tag_count - 1);
      u64 offset = file->header.tag_list + sizeof(Hha_tag);
      
      platform.read_data_from_file(file->handle, offset, tag_array_size, asset_list->tag_list + file->tag_base);
    }
  }
  
  u32 asset_count = 0;
  mem_zero_struct(*(asset_list->asset_list + asset_count));
  asset_count++;
  
  for (u32 dst_type_id = 0; dst_type_id < Asset_count; dst_type_id++) {
    Asset_type *dst_type = asset_list->asset_type_list + dst_type_id;
    dst_type->first_asset_index = asset_count;
    
    for (u32 file_index = 0; file_index < asset_list->file_count; file_index++) {
      Asset_file *file = asset_list->file_list + file_index;
      
      assert(file->handle->no_errors);
      
      for (u32 source_index = 0; source_index < file->header.asset_type_count; source_index++) {
        Hha_asset_type *source_type = file->asset_type_array + source_index;
        
        if (source_type->type_id != dst_type_id)
          continue;
        
        u32 asset_count_for_type = source_type->one_past_last_asset_index - source_type->first_asset_index;
        
        Temp_memory temp_mem = begin_temp_memory(&tran_state->arena);
        
        Hha_asset *hha_asset_array = mem_push_array(&tran_state->arena, asset_count_for_type, Hha_asset);
        
        platform.read_data_from_file(file->handle, file->header.asset_list + source_type->first_asset_index * sizeof(Hha_asset),
                                     asset_count_for_type * sizeof (Hha_asset),
                                     hha_asset_array);
        
        for (u32 asset_index = 0; asset_index < asset_count_for_type; asset_index++) {
          Hha_asset *hha_asset = hha_asset_array + asset_index;
          
          assert(asset_count < asset_list->asset_count);
          Asset *asset = asset_list->asset_list + asset_count++;
          
          asset->file_index = file_index;
          asset->hha = *hha_asset;
          
          if (asset->hha.first_tag_index == 0) {
            asset->hha.first_tag_index = asset->hha.one_past_last_tag_index = 0;
          }
          else {
            asset->hha.first_tag_index += (file->tag_base - 1);
            asset->hha.one_past_last_tag_index += (file->tag_base - 1);
          }
          
        }
        
        end_temp_memory(temp_mem);
      }
    }
    
    dst_type->one_past_last_asset_index = asset_count;
  }
  
  return asset_list;
}