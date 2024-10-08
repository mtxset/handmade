#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "types.h"
#include "platform.h"

#include "intrinsics.h"
#include "math.h"

#include "asset_type_id.h"
#include "file_formats.h"

#define _(x)

FILE *out = 0;

#define assert(expr) if (!(expr)) {*(i32*)0 = 0;}


#pragma pack(push, 1)
// struct is from: https://www.fileformat.info/format/bmp/egff.htm
// preventing compiler padding this struct
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

#define RIFF_CODE(a,b,c,d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

// IFF - created by ea in amiga, big-endian

enum {
  Wave_ChunkID_fmt  = RIFF_CODE('f', 'm', 't', ' '),
  Wave_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
  Wave_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
  Wave_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a')
};

struct Wave_chunk {
  u32 id;
  u32 size;
};

struct Wave_header {
  u32 riffid;
  u32 size;
  u32 waveid;
};

struct Wave_fmt {
  u16 format_tag;
  u16 channels;
  u32 samples_per_sec;
  u32 avg_bytes_per_sec;
  u16 block_align;
  u16 bits_per_sample;
  u16 size;
  u16 valid_bits_per_sample;
  u32 channel_mask;
  u8 sub_format[16];
};

#pragma pack(pop)

struct File_read_result {
  u32 bytes_read;
  void *content;
};

struct Loaded_sound {
  u32 sample_count; // sample count divided by 8, cuz simd (4 * channel count)
  u32 channel_count;
  
  i16 *samples[2];
  
  void *free;
};

struct Loaded_bmp {
  i32 width;
  i32 height;
  i32 pitch;
  
  void *memory;
  void *free;
};

enum Asset_type {
  Asset_type_sound,
  Asset_type_bitmap,
};

struct Asset_source {
  Asset_type type;
  char *filename;
  u32 first_sample_index;
};

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
  Asset_source asset_source_list[MAX_COUNT];
  Hha_asset asset_list[MAX_COUNT];
  
  Hha_asset_type *debug_asset_type;
  u32 asset_index;
};

static
File_read_result
read_entire_file(char *filename) {
  File_read_result result = {};
  
  FILE *read = 0;
  errno_t open_result = fopen_s(&read, filename, "rb");
  
  if (open_result == 0) {
    fseek(read, 0, SEEK_END);
    result.bytes_read = ftell(read);
    fseek(read, 0, SEEK_SET);
    
    result.content = malloc(result.bytes_read);
    fread(result.content, result.bytes_read, _(count)1, read);
    fclose(read);
  }
  else {
    printf("Can't open file %s\n", filename);
  }
  
  return result;
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

static
Loaded_bmp
load_bmp(char* filename) {
  Loaded_bmp result = {};
  
  File_read_result file_result = read_entire_file(filename);
  assert(file_result.bytes_read != 0);
  
  result.free = file_result.content;
  
  Bitmap_header* header = (Bitmap_header*)file_result.content;
  
  assert(result.height >= 0);
  assert(header->Compression == 3);
  
  result.memory = (u32*)((u8*)file_result.content + header->BitmapOffset);
  result.width  = header->Width;
  result.height = header->Height;
  
  u32 mask_red   = header->RedMask;
  u32 mask_green = header->GreenMask;
  u32 mask_blue  = header->BlueMask;
  u32 mask_alpha = ~(mask_red | mask_green | mask_blue);
  
  Bit_scan_result scan_red   = find_least_significant_first_bit(mask_red);
  Bit_scan_result scan_green = find_least_significant_first_bit(mask_green);
  Bit_scan_result scan_blue  = find_least_significant_first_bit(mask_blue);
  Bit_scan_result scan_alpha = find_least_significant_first_bit(mask_alpha);
  
  assert(scan_red.found);
  assert(scan_green.found);
  assert(scan_blue.found);
  assert(scan_alpha.found);
  
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

static
Loaded_sound
load_wav(char *filename, u32 section_first_sample_index, u32 section_sample_count) {
  Loaded_sound result = {};
  
  File_read_result read_result = read_entire_file(filename);
  assert(read_result.content);
  
  result.free = read_result.content;
  
  Wave_header *header = (Wave_header*)read_result.content;
  assert(header->riffid == Wave_ChunkID_RIFF);
  assert(header->waveid == Wave_ChunkID_WAVE);
  
  u32 channel_count = 0;
  u32 sample_data_size = 0;
  i16 *sample_data = 0;
  
  for (Riff_iterator iter = parse_chunk_at(header + 1, (u8*)(header + 1) + header->size - 4);
       iter.at < iter.stop;
       iter = next_chunk(iter)) {
    
    switch (get_type(iter)) {
      
      case Wave_ChunkID_fmt: {
        Wave_fmt *fmt = (Wave_fmt*)get_chunk_data(iter);
        assert(fmt->format_tag == 1);
        assert(fmt->samples_per_sec == 48000);
        assert(fmt->bits_per_sample == 16);
        assert(fmt->block_align == sizeof(i16) * fmt->channels);
        channel_count = fmt->channels;
        u32 u = 0;
      } break;
      
      case Wave_ChunkID_data: {
        sample_data = (i16*)get_chunk_data(iter);
        sample_data_size = get_chunk_data_size(iter);
      } break;
      
      //default: assert(!"FAILURE");
    }
    
  }
  
  assert(channel_count && sample_data);
  
  result.channel_count = channel_count;
  u32 sample_count = sample_data_size / (channel_count * sizeof(i16));
  
  if (channel_count == 1) {
    result.samples[0] = sample_data;
    result.samples[1] = 0;
  } 
  else if (channel_count == 2) {
    
    result.samples[0] = sample_data;
    result.samples[1] = sample_data + sample_count;
    
    for (u32 sample_index = 0; sample_index < sample_count; sample_index++) {
      i16 source = sample_data[2 * sample_index];
      
      sample_data[2 * sample_index] = sample_data[sample_index];
      sample_data[sample_index] = source;
    }
    
  }
  else {
    assert(!"CANT BE");
  }
  
  bool at_end = true;
  result.channel_count = 1;
  
  if (section_sample_count) {
    assert((section_first_sample_index + section_sample_count) <= sample_count);
    at_end = (section_first_sample_index + section_sample_count == sample_count);
    sample_count = section_sample_count;
    
    for (u32 channel_index = 0; channel_index < result.channel_count; channel_index++) {
      result.samples[channel_index] += section_first_sample_index;
    }
  }
  
  if (at_end) {
    
    for (u32 channel_index = 0; channel_index < result.channel_count; channel_index++) {
      for (u32 sample_index = 0; sample_index < result.channel_count + 8; sample_index++) {
        result.samples[channel_index][sample_index] = 0;
      }
    }
  }
  
  result.sample_count = sample_count;
  
  return result;
}


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
  asset_list->asset_index = 0;
}

static
Bitmap_id
add_bitmap_asset(Game_asset_list* asset_list, char *filename, f32 align_pcent_x = .5f, f32 align_pcent_y = .5f) {
  assert(asset_list->debug_asset_type);
  assert(asset_list->debug_asset_type->one_past_last_asset_index < array_count(asset_list->asset_list));
  
  Bitmap_id id = {asset_list->debug_asset_type->one_past_last_asset_index++};
  Asset_source *source = asset_list->asset_source_list + id.value;
  
  Hha_asset *asset = asset_list->asset_list + id.value;
  
  asset->first_tag_index = asset_list->tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->bitmap.align_pcent[0] = align_pcent_x;
  asset->bitmap.align_pcent[1] = align_pcent_y;
  
  source->type = Asset_type_bitmap;
  source->filename = filename;
  
  asset_list->asset_index = id.value;
  
  return id;
}

static
Sound_id
add_sound_asset(Game_asset_list* asset_list, char *filename, u32 first_sample_index = 0, u32 sample_count = 0) {
  assert(asset_list->debug_asset_type);
  assert(asset_list->debug_asset_type->one_past_last_asset_index < array_count(asset_list->asset_list));
  
  Sound_id id = {asset_list->debug_asset_type->one_past_last_asset_index++};
  Asset_source *source = asset_list->asset_source_list + id.value;
  
  Hha_asset *asset = asset_list->asset_list + id.value;
  
  asset->first_tag_index = asset_list->tag_count;
  asset->one_past_last_tag_index = asset->first_tag_index;
  asset->sound.sample_count = sample_count;
  asset->sound.chain = Hha_sound_chain_none;
  
  source->type = Asset_type_sound;
  source->filename = filename;
  source->first_sample_index = first_sample_index;
  
  asset_list->asset_index = id.value;
  
  return id;
}

static
void
add_tag(Game_asset_list *asset_list, Asset_tag_id id, f32 value) {
  assert(asset_list->asset_index);
  
  Hha_asset *asset = asset_list->asset_list + asset_list->asset_index;
  asset->one_past_last_tag_index++;
  
  Hha_tag *tag = asset_list->tag_list + asset_list->tag_count++;
  
  tag->id = id;
  tag->value = value;
}

static
void
init(Game_asset_list *asset_list) {
  
  asset_list->tag_count = 1;
  asset_list->asset_count = 1;
  asset_list->debug_asset_type = 0;
  asset_list->asset_index = 0;
  
  asset_list->asset_type_count = Asset_count;
  memset(asset_list->asset_type_list, 0, sizeof(asset_list->asset_type_list));
}

static
void
write_hha_file(Game_asset_list *asset_list, char *filename) {
  
  errno_t open_result = fopen_s(&out, filename, "wb");
  
  if (open_result == 0) {
    
    Hha_header header = {};
    
    header.magic_value = HHA_MAGIC_VALUE;
    header.version = HHA_VERSION;
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
    
    fseek(out, asset_array_size, SEEK_CUR);
    
    for (u32 asset_index = 1; asset_index < header.asset_count; asset_index++) {
      Asset_source *src = asset_list->asset_source_list + asset_index;
      Hha_asset    *dst = asset_list->asset_list + asset_index;
      
      dst->data_offset = ftell(out);
      
      if (src->type == Asset_type_sound) {
        Loaded_sound wav = load_wav(src->filename, src->first_sample_index, dst->sound.sample_count);
        
        dst->sound.sample_count = wav.sample_count;
        dst->sound.channel_count = wav.channel_count;
        
        for (u32 channel_index = 0; channel_index < wav.channel_count; channel_index++) {
          size_t write_bytes = dst->sound.sample_count * sizeof(i16);
          fwrite(wav.samples[channel_index], write_bytes, _(count)1, out);
        }
        
        free(wav.free);
      }
      else {
        assert(src->type == Asset_type_bitmap);
        
        Loaded_bmp bitmap = load_bmp(src->filename);
        
        dst->bitmap.dim[0] = bitmap.width;
        dst->bitmap.dim[1] = bitmap.height;
        
        assert(bitmap.width * 4 == bitmap.pitch);
        
        size_t write_bytes = bitmap.width * bitmap.height * 4;
        fwrite(bitmap.memory, write_bytes, _(count)1, out);
        
        free(bitmap.free);
      }
      
    }
    
    fseek(out, (u32)header.asset_list, SEEK_SET);
    fwrite(asset_list->asset_list, asset_array_size, _(count)1, out);
    fclose(out);
  }
  else {
    printf("can't open file\n");
  }
  
}

static
void
write_non_hero() {
  
  Game_asset_list _asset_list_;
  Game_asset_list *asset_list = &_asset_list_;
  
  init(asset_list);
  
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
  
  begin_asset_type(asset_list, Asset_tree);
  add_bitmap_asset(asset_list, "../data/tree.bmp");
  end_asset_type(asset_list);
  
  write_hha_file(asset_list, "test2.hha");
}

static
void
write_hero() {
  
  Game_asset_list _asset_list_;
  Game_asset_list *asset_list = &_asset_list_;
  
  init(asset_list);
  
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
  
  write_hha_file(asset_list, "test1.hha");
}


static
void
write_sounds() {
  
  Game_asset_list _asset_list_;
  Game_asset_list *asset_list = &_asset_list_;
  
  init(asset_list);
  
  {
    u32 music_chunk = 48000 * 10;
    u32 total_music_sample_count = 7468095;
    begin_asset_type(asset_list, Asset_music);
    Sound_id last_piece = {};
    char *path_to_music = "../data/sounds/music_test.wav";
    for (u32 first_sample_index = 0; first_sample_index < total_music_sample_count; first_sample_index += music_chunk) {
      
      u32 sample_count = total_music_sample_count - first_sample_index;
      
      if (sample_count > music_chunk) {
        sample_count = music_chunk;
      }
      
      Sound_id this_piece = add_sound_asset(asset_list, path_to_music, first_sample_index, sample_count);
      
      if (first_sample_index + music_chunk < total_music_sample_count) {
        asset_list->asset_list[last_piece.value].sound.chain = Hha_sound_chain_advance;
      }
      
      last_piece = this_piece;
    }
    
    end_asset_type(asset_list);
  }
  
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
  
  write_hha_file(asset_list, "test3.hha");
}


int
main(int arg_count, char **arg_list) {
  
  write_non_hero();
  write_hero();
  write_sounds();
  
  return 0;
}