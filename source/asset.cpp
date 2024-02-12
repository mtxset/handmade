// struct is from: https://www.fileformat.info/format/bmp/egff.htm
// preventing compiler padding this struct
#pragma pack(push, 1)
struct Bitmap_header {
  u16 FileType;        /* File type, always 4D42h ("BM") */
  u32 FileSize;        /* Size of the file in bytes */
  u16 Reserved1;       /* Always 0 */
  u16 Reserved2;       /* Always 0 */
  u32 BitmapOffset;    /* Starting position of image data in bytes */
  
  u32 Size;            /* Size of this header in bytes */
  i32 Width;           /* Image width in pixels */
  i32 Height;          /* Image height in pixels */
  u16 Planes;          /* Number of color planes */
  u16 BitsPerPixel;    /* Number of bits per pixel */
  u32 Compression;     /* Compression methods used */
  u32 SizeOfBitmap;    /* Size of bitmap in bytes */
  i32 HorzResolution;  /* Horizontal resolution in pixels per meter */
  i32 VertResolution;  /* Vertical resolution in pixels per meter */
  u32 ColorsUsed;      /* Number of colors in the image */
  u32 ColorsImportant; /* Minimum number of important colors */
  
  u32 RedMask;         /* Mask identifying bits of red component */
  u32 GreenMask;       /* Mask identifying bits of green component */
  u32 BlueMask;        /* Mask identifying bits of blue component */
};
#pragma pack(pop)

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
debug_load_bmp(char* file_name, i32 align_x, i32 top_down_align_y) {
  Loaded_bmp result = {};
  
  Debug_file_read_result file_result = debug_read_entire_file(file_name);
  
  macro_assert(file_result.bytes_read != 0);
  
  Bitmap_header* header = (Bitmap_header*)file_result.content;
  
  macro_assert(result.height >= 0);
  macro_assert(header->Compression == 3);
  
  result.memory = (u32*)((u8*)file_result.content + header->BitmapOffset);
  
  result.width  = header->Width;
  result.height = header->Height;
  result.align_pcent = top_down_align(&result, v2_i32(align_x, top_down_align_y));
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

internal
Loaded_bmp
debug_load_bmp(char* file_name) {
  Loaded_bmp result = debug_load_bmp(file_name, 0, 0);
  result.align_pcent = v2{0.5f, 0.5f};
  
  return result;
}

struct Load_bitmap_work {
  Game_asset_list *asset_list;
  char *file_name;
  Bitmap_id id;
  Task_with_memory *task;
  Loaded_bmp *bitmap;
  
  bool has_alignment;
  i32 align_x;
  i32 top_down_align_y;
  
  Asset_state final_state;
};


internal 
PLATFORM_WORK_QUEUE_CALLBACK(load_bitmap_work) {
  Load_bitmap_work *work = (Load_bitmap_work*)data;
  
  if (work->has_alignment) {
    *work->bitmap = debug_load_bmp(work->file_name, work->align_x, work->top_down_align_y);
  }
  else {
    *work->bitmap = debug_load_bmp(work->file_name);
  }
  
  _WriteBarrier();
  
  work->asset_list->bitmap_list[work->id.value].bitmap = work->bitmap;
  work->asset_list->bitmap_list[work->id.value].state = work->final_state;
  
  end_task_with_mem(work->task);
}


internal
void 
load_bitmap(Game_asset_list *asset_list, Bitmap_id id) {
  
  if (atomic_compare_exchange_u32((u32*)&asset_list->bitmap_list[id.value].state, Asset_state_unloaded, Asset_state_queued) != Asset_state_unloaded) 
    return;
  
  Task_with_memory *task = begin_task_with_mem(asset_list->tran_state);
  
  if (!task)
    return;
  
  Load_bitmap_work *work = mem_push_struct(&task->arena, Load_bitmap_work);
  
  work->asset_list = asset_list;
  work->id = id;
  work->file_name = "";
  work->task = task;
  work->bitmap = mem_push_struct(&asset_list->arena, Loaded_bmp);
  work->has_alignment = false;
  work->final_state = Asset_state_loaded;
  
  switch (id.value) {
    case Asset_background: {
      work->file_name = "../data/bg_nebula.bmp";
    } break;
    
    case Asset_tree: { 
      work->file_name = "../data/tree.bmp";
      work->has_alignment = true;
      work->align_x = 50;
      work->top_down_align_y = 115;
    } break;
    
    case Asset_monster: {
      work->file_name = "../data/george-front-0.bmp";
      work->has_alignment = true;
      work->align_x = 25;
      work->top_down_align_y = 42;
    } break;
    
    case Asset_familiar: {
      work->file_name = "../data/ship.bmp";
      work->has_alignment = true;
      work->align_x = 34;
      work->top_down_align_y = 72;
    } break;
    
    case Asset_sword: {
      work->file_name = "../data/sword.bmp";
      work->has_alignment = true;
      work->align_x = 33;
      work->top_down_align_y = 29;
    } break;
    
    case Asset_stairwell: {
      work->file_name = "../data/george-back-0.bmp";
    } break;
  }
  
  platform_add_entry(asset_list->tran_state->low_priority_queue, load_bitmap_work, work);
}

internal void
load_sound(Game_asset_list *asset_list, u32 id) {
}

internal
Bitmap_id
get_first_bitmap_id(Game_asset_list *asset_list, Asset_type_id type_id) {
  Bitmap_id result = {};
  
  Asset_type *type = asset_list->asset_type_list + type_id;
  
  if (type->first_asset_index != type->one_past_last_asset_index) {
    Asset *asset = asset_list->asset_list + type->first_asset_index;
    result.value = asset->slot_id;
  }
  
  return result;
}

internal
Game_asset_list*
allocate_game_asset_list(Memory_arena *arena, size_t size, Transient_state *tran_state) {
  
  Game_asset_list *asset_list = mem_push_struct(arena, Game_asset_list);
  sub_arena(&asset_list->arena, arena, size);
  asset_list->tran_state = tran_state;
  
  asset_list->bitmap_count = Asset_count;
  asset_list->bitmap_list = mem_push_array(arena, asset_list->bitmap_count, Asset_slot);
  
  asset_list->sound_count = 1;
  asset_list->sound_list = mem_push_array(arena, asset_list->sound_count, Asset_slot);
  
  asset_list->tag_count = 0;
  asset_list->tag_list = 0;
  
  asset_list->asset_count = asset_list->bitmap_count;
  asset_list->asset_list = mem_push_array(arena, asset_list->asset_count, Asset);
  
  for (u32 asset_id = 0; asset_id < Asset_count; asset_id++) {
    Asset_type *type = asset_list->asset_type_list + asset_id;
    
    type->first_asset_index = asset_id;
    type->one_past_last_asset_index = asset_id + 1;
    
    Asset *asset = asset_list->asset_list + type->first_asset_index;
    asset->first_tag_index = 0;
    asset->one_past_last_tag_index = 0;
    asset->slot_id = type->first_asset_index;
  }
  
  asset_list->grass[0]   = debug_load_bmp("../data/grass00.bmp");
  asset_list->grass[1]   = debug_load_bmp("../data/grass01.bmp");
  
  asset_list->tuft[0]    = debug_load_bmp("../data/tuft00.bmp");
  asset_list->tuft[1]    = debug_load_bmp("../data/tuft01.bmp");
  asset_list->tuft[2]    = debug_load_bmp("../data/tuft02.bmp");
  
  asset_list->stone[0]   = debug_load_bmp("../data/ground00.bmp");
  asset_list->stone[1]   = debug_load_bmp("../data/ground01.bmp");
  asset_list->stone[2]   = debug_load_bmp("../data/ground02.bmp");
  asset_list->stone[3]   = debug_load_bmp("../data/ground03.bmp");
  
  Hero_bitmaps* bitmap = asset_list->hero_bitmaps;
  
  bitmap->head  = debug_load_bmp("../data/test_hero_right_head.bmp");
  bitmap->cape  = debug_load_bmp("../data/test_hero_right_cape.bmp");
  bitmap->torso = debug_load_bmp("../data/test_hero_right_torso.bmp");
  set_top_down_align(bitmap, v2{72, 182});
  bitmap++;
  
  bitmap->head  = debug_load_bmp("../data/test_hero_back_head.bmp");
  bitmap->cape  = debug_load_bmp("../data/test_hero_back_cape.bmp");
  bitmap->torso = debug_load_bmp("../data/test_hero_back_torso.bmp");
  set_top_down_align(bitmap, v2{72, 182});
  bitmap++;
  
  bitmap->head  = debug_load_bmp("../data/test_hero_left_head.bmp");
  bitmap->cape  = debug_load_bmp("../data/test_hero_left_cape.bmp");
  bitmap->torso = debug_load_bmp("../data/test_hero_left_torso.bmp");
  set_top_down_align(bitmap, v2{72, 182});
  bitmap++;
  
  bitmap->head  = debug_load_bmp("../data/test_hero_front_head.bmp");
  bitmap->cape  = debug_load_bmp("../data/test_hero_front_cape.bmp");
  bitmap->torso = debug_load_bmp("../data/test_hero_front_torso.bmp");
  set_top_down_align(bitmap, v2{72, 182});
  
  return asset_list;
}