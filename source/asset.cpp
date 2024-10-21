struct Load_asset_work {
  Task_with_memory *task;
  Asset *asset;
  
  Platform_file_handle *handle;
  
  u64 offset;
  u64 size;
  
  void *destination;
  
  u32 final_state;
};


struct Asset_memory_size {
  u32 total;
  u32 data;
  u32 section;
};

inline
void 
insert_asset_header_at_front(Game_asset_list* asset_list, Asset_memory_header *header) {
  Asset_memory_header *sentinel = &asset_list->loaded_asset_sentinel;
  
  header->prev = sentinel;
  header->next = sentinel->next;
  
  header->next->prev = header;
  header->prev->next = header;
}

static 
void
add_asset_header_to_list(Game_asset_list *asset_list, u32 asset_index, Asset_memory_size size) {
  
  Asset *asset = asset_list->asset_list + asset_index;
  
  Asset_memory_header *header = asset->header;
  
  header->asset_index = asset_index;
  header->total_size = size.total;
  
  insert_asset_header_at_front(asset_list, header);
}

static
void
remove_asset_header_from_list(Asset_memory_header *header) {
  header->prev->next = header->next;
  header->next->prev = header->prev;
  
  header->next = header->prev = 0;
}

void
move_header_to_front(Game_asset_list *asset_list, Asset *asset) {
  
  if (!is_locked(asset)) {
    Asset_memory_header *header = asset->header;
    
    remove_asset_header_from_list(header);
    insert_asset_header_at_front(asset_list, header);
  }
  
}

internal 
PLATFORM_WORK_QUEUE_CALLBACK(load_asset_work) {
  Load_asset_work *work = (Load_asset_work*)data;
  
  platform.read_data_from_file(work->handle, work->offset, work->size, work->destination);
  
  _WriteBarrier();
  
  if (!work->handle->no_errors) {
    mem_zero_size(work->size, work->destination);
  }
  
  work->asset->state = work->final_state;
  
  end_task_with_mem(work->task);
}

inline 
Platform_file_handle*
get_file_handle_for(Game_asset_list *asset_list, u32 file_index)
{
  assert(file_index < asset_list->file_count);
  
  Platform_file_handle *result = &asset_list->file_list[file_index].handle;
  
  return result;
}

static
Asset_memory_block*
find_block_for_size(Game_asset_list *asset_list, sz size) {
  
  Asset_memory_block* result = 0;
  
  for (Asset_memory_block *block = asset_list->memory_sentnel.next;
       block != &asset_list->memory_sentnel;
       block = block->next) {
    
    if (! (block->flags & Asset_memory_block_used)) {
      
      if (block->size >= size) {
        result = block;
        break;
      }
      
    }
    
  }
  
  return result;
}

static
Asset_memory_block*
insert_block(Asset_memory_block *prev, u64 size, void *memory) {
  assert(size > sizeof(Asset_memory_block));
  
  Asset_memory_block *result = (Asset_memory_block*)memory;
  
  result->flags = 0;
  result->size  = size - sizeof(Asset_memory_block);
  result->prev  = prev;
  result->next  = prev->next;
  result->prev->next = result;
  result->next->prev = result;
  
  return result;
}

static
bool
merge_if_possible(Game_asset_list *asset_list, Asset_memory_block *first, Asset_memory_block *second) {
  
  if (first == &asset_list->memory_sentnel ||
      second == &asset_list->memory_sentnel) {
    return false;
  }
  
  if (first->flags & Asset_memory_block_used ||
      second->flags & Asset_memory_block_used) {
    return false;
  }
  
  u8 *expected_second = (u8*)first + sizeof(Asset_memory_block) + first->size;
  
  if ((u8*)second == expected_second) {
    second->next->prev = second->prev;
    second->prev->next = second->next;
    
    first->size += sizeof(Asset_memory_block) + second->size;
    
    return true;
  }
  
  return false;
}

inline 
void*
acquire_asset_memory(Game_asset_list *asset_list, sz size) {
  
  void *result = 0;
  
  Asset_memory_block *block = find_block_for_size(asset_list, size);
  while (true) {
    
    if (block && size <= block->size) {
      
      block->flags |= Asset_memory_block_used;
      
      result = (u8*)(block + 1);
      
      sz remaining_size = block->size - size;
      sz block_split_threshold = 4096;
      
      if (remaining_size > block_split_threshold) {
        block->size -= remaining_size;
        insert_block(block, remaining_size, (u8*)result + size);
      }
      else {
        // unused blocks
      }
      
      break;
      
    }
    else {
      
      for (Asset_memory_header *header = asset_list->loaded_asset_sentinel.prev;
           header != &asset_list->loaded_asset_sentinel;
           header = header->prev) {
        
        Asset *asset = asset_list->asset_list + header->asset_index;
        
        if (get_state(asset) >= Asset_state_loaded) {
          
          assert(get_state(asset) == Asset_state_loaded);
          assert(!is_locked(asset));
          
          remove_asset_header_from_list(header);
          
          block = (Asset_memory_block*)asset->header - 1;
          block->flags &= ~Asset_memory_block_used;
          
          if (merge_if_possible(asset_list, block->prev, block)) {
            block = block->prev;
          }
          
          merge_if_possible(asset_list, block, block->next);
          
          asset->state = Asset_state_unloaded;
          asset->header = 0;
          break;
        }
        
      }
      
    }
  }
  
  return result;
}

void 
load_bitmap(Game_asset_list *asset_list, Bitmap_id id, bool locked) {
  
  Asset *asset = asset_list->asset_list + id.value;
  
  if (id.value &&
      atomic_compare_exchange_u32((u32*)&asset->state, Asset_state_queued, Asset_state_unloaded) == Asset_state_unloaded) {
    
    Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
    
    if (task) {
      
      Hha_bitmap *info = &asset->hha.bitmap;
      
      u32 width  = info->dim[0];
      u32 height = info->dim[1];
      
      Asset_memory_size size = {};
      size.section = 4 * width;
      size.data = height * size.section;
      size.total = size.data + sizeof(Asset_memory_header);
      
      asset->header = (Asset_memory_header*)acquire_asset_memory(asset_list, size.total);
      
      Loaded_bmp *bitmap = &asset->header->bitmap;
      bitmap->align_pcent = {info->align_pcent[0], info->align_pcent[1]};
      bitmap->width_over_height = (f32)info->dim[0] / (f32)info->dim[1];
      bitmap->width = info->dim[0];
      bitmap->height = info->dim[1];
      bitmap->pitch = size.section;
      bitmap->memory = (asset->header + 1);
      
      Load_asset_work *work = mem_push_struct(&task->arena, Load_asset_work);
      work->task = task;
      work->asset = asset;
      work->handle = get_file_handle_for(asset_list, asset->file_index);
      work->offset = asset->hha.data_offset;
      work->size = size.data;
      work->destination = bitmap->memory;
      work->final_state = Asset_state_loaded | (locked ? Asset_state_lock : 0);
      
      asset->state |= Asset_state_lock;
      
      if (!locked) {
        add_asset_header_to_list(asset_list, id.value, size);
      }
      
      platform.add_entry(asset_list->tran_state->low_priority_queue, load_asset_work, work);
    }
    else {
      asset->state = Asset_state_unloaded;
    }
    
  }
}


void
load_sound(Game_asset_list *asset_list, Sound_id id) {
  
  Asset *asset = asset_list->asset_list + id.value;
  
  if (id.value &&
      (atomic_compare_exchange_u32((u32*)&asset->state, Asset_state_queued, Asset_state_unloaded) == Asset_state_unloaded)) {
    
    Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
    
    if (task) {
      Hha_sound *info = &asset->hha.sound;
      
      Asset_memory_size size = {};
      size.section = info->sample_count * sizeof(i16);
      size.data = info->channel_count * size.section;
      size.total = size.data + sizeof(Asset_memory_header);
      
      asset->header = (Asset_memory_header*)acquire_asset_memory(asset_list, size.total);
      Loaded_sound *sound = &asset->header->sound;
      
      sound->sample_count = info->sample_count;
      sound->channel_count = info->channel_count;
      u32 channel_size = size.section;
      
      void *memory = (asset->header + 1);
      i16 *sound_at = (i16*)memory;
      for (u32 channel_index = 0; channel_index < sound->channel_count; channel_index++) {
        sound->samples[channel_index] = sound_at;
        sound_at += channel_size;
      }
      
      Load_asset_work *work = mem_push_struct(&task->arena, Load_asset_work);
      
      work->task = task;
      work->asset = asset;
      work->handle = get_file_handle_for(asset_list, asset->file_index);
      work->offset = asset->hha.data_offset;
      work->size = size.data;
      work->destination = memory;
      work->final_state = Asset_state_loaded;
      
      add_asset_header_to_list(asset_list, id.value, size);
      
      platform.add_entry(asset_list->tran_state->low_priority_queue, load_asset_work, work);
    }
    else {
      asset->state = Asset_state_unloaded;
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
  
  asset_list->tran_state = tran_state;
  
  asset_list->memory_sentnel.flags = 0;
  asset_list->memory_sentnel.size = 0;
  asset_list->memory_sentnel.next = &asset_list->memory_sentnel;
  asset_list->memory_sentnel.prev = &asset_list->memory_sentnel;
  
  insert_block(&asset_list->memory_sentnel, size, mem_push_size(arena, size));
  
  for (u32 tag_type = 0; tag_type < Tag_count; tag_type++) {
    asset_list->tag_range[tag_type] = 1000000.0f;
  }
  asset_list->tag_range[Tag_facing_dir] = TAU;
  
  asset_list->tag_count = 1;
  asset_list->asset_count = 1;
  
  asset_list->loaded_asset_sentinel.next = &asset_list->loaded_asset_sentinel;
  asset_list->loaded_asset_sentinel.prev = &asset_list->loaded_asset_sentinel;
  
  Platform_file_group file_group = platform.get_all_files_of_type_begin(Platform_file_type_asset);
  asset_list->file_count = file_group.file_count;
  asset_list->file_list = mem_push_array(arena, asset_list->file_count, Asset_file);
  
  for (u32 file_index = 0; file_index < asset_list->file_count; file_index++) {
    Asset_file *file = asset_list->file_list + file_index;
    
    file->tag_base = asset_list->tag_count;
    
    mem_zero_struct(file->header);
    file->handle = platform.open_next_file(&file_group);
    platform.read_data_from_file(&file->handle, 0, sizeof(file->header), &file->header);
    
    u32 asset_type_array_size = file->header.asset_type_count * sizeof(Hha_asset_type);
    file->asset_type_array = (Hha_asset_type*)mem_push_size(arena, asset_type_array_size);
    platform.read_data_from_file(&file->handle, file->header.asset_type_list, asset_type_array_size, file->asset_type_array);
    
    assert(file->handle.no_errors);
    
    if (file->header.magic_value != HHA_MAGIC_VALUE) {
      platform.file_error(&file->handle, "wrong magic value");
    }
    
    if (file->header.version > HHA_VERSION) {
      platform.file_error(&file->handle, "unsupperted version");
    }
    
    // skip first cuz it's null
    asset_list->tag_count   += (file->header.tag_count - 1);
    asset_list->asset_count += (file->header.asset_count - 1);
  }
  platform.get_all_files_of_type_end(&file_group);
  
  asset_list->asset_list = mem_push_array(arena, asset_list->asset_count, Asset);
  asset_list->tag_list   = mem_push_array(arena, asset_list->tag_count, Asset_tag);
  
  mem_zero_struct(asset_list->tag_list[0]);
  
  for (u32 file_index = 0; file_index < asset_list->file_count; file_index++) {
    Asset_file *file = asset_list->file_list + file_index;
    
    if (file->handle.no_errors) {
      u32 tag_array_size = sizeof(Hha_tag) * (file->header.tag_count - 1);
      u64 offset = file->header.tag_list + sizeof(Hha_tag);
      
      platform.read_data_from_file(&file->handle, offset, tag_array_size, asset_list->tag_list + file->tag_base);
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
      
      assert(file->handle.no_errors);
      
      for (u32 source_index = 0; source_index < file->header.asset_type_count; source_index++) {
        Hha_asset_type *source_type = file->asset_type_array + source_index;
        
        if (source_type->type_id != dst_type_id)
          continue;
        
        u32 asset_count_for_type = source_type->one_past_last_asset_index - source_type->first_asset_index;
        
        Temp_memory temp_mem = begin_temp_memory(&tran_state->arena);
        
        Hha_asset *hha_asset_array = mem_push_array(&tran_state->arena, asset_count_for_type, Hha_asset);
        
        platform.read_data_from_file(&file->handle, file->header.asset_list + source_type->first_asset_index * sizeof(Hha_asset),
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