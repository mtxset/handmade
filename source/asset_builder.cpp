#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "types.h"
#include "platform.h"

#include "intrinsics.h"
#include "math.h"

#include "asset_type_id.h"
#include "file_formats.h"


#define USE_FONTS_FROM_WINDOWS 1

#if USE_FONTS_FROM_WINDOWS

#include <windows.h>

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)
#define MAX_FONT_WIDTH 1024
#define MAX_FONT_HEIGHT 1024

static VOID *global_font_bits;
static HDC global_font_device_context;

#else
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#endif

#undef assert

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

struct Loaded_font {
  HFONT win32_handle;
  TEXTMETRIC text_metric;
  f32 line_advance;
  
  Hha_font_glyph *glyph_list;
  f32 *horizontal_advance;
  
  //u32 min_code_point;
  //u32 max_code_point;
  
  u32 max_glyph_count;
  u32 glyph_count;
  
  u32 *glyph_index_from_code_point;
};

enum Asset_type {
  Asset_type_sound,
  Asset_type_bitmap,
  Asset_type_font,
  Asset_type_font_glyph,
};

struct Asset_source_bitmap {
  char *filename;
};

struct Asset_source_sound {
  char *filename;
  u32 first_sample_index;
};

struct Asset_source_font {
  Loaded_font *font;
};

struct Asset_source_font_glyph {
  Loaded_font *font;
  u32 code_point;
};

struct Asset_source {
  Asset_type type;
  
  union {
    Asset_source_bitmap bitmap;
    Asset_source_sound sound;
    Asset_source_font font;
    Asset_source_font_glyph glyph;
  };
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
Loaded_bmp
load_glyph(Loaded_font *font, u32 code_point, Hha_asset *asset) {
  
  Loaded_bmp bmp = {};
  
  u32 glyph_index = font->glyph_index_from_code_point[code_point];
  
#if USE_FONTS_FROM_WINDOWS
  
  SelectObject(global_font_device_context, font->win32_handle);
  memset(global_font_bits, 0x00, MAX_FONT_HEIGHT * MAX_FONT_WIDTH * sizeof(u32));
  wchar_t cheese_point = (wchar_t)code_point;
  
  SIZE size;
  GetTextExtentPoint32W(global_font_device_context, &cheese_point, 1, &size);
  
  i32 pre_step_x = 128;
  i32 bound_width = size.cx + 2 * pre_step_x;
  i32 bound_height = size.cy;
  
  if (bound_width  > MAX_FONT_WIDTH)  bound_width  = MAX_FONT_WIDTH;
  if (bound_height > MAX_FONT_HEIGHT) bound_height = MAX_FONT_HEIGHT;
  
  SetTextColor(global_font_device_context, RGB(255, 255, 255));
  TextOutW(global_font_device_context, pre_step_x, 0, &cheese_point, 1);
  
  i32 min_x = 10000;
  i32 min_y = 10000;
  i32 max_x = -10000;
  i32 max_y = -10000;
  
  u32 *row = (u32*)global_font_bits + (MAX_FONT_HEIGHT - 1) * MAX_FONT_WIDTH;
  
  for (i32 y = 0; y < bound_height; y++) {
    u32 *pixel = row;
    for (i32 x = 0; x < bound_width; x++) {
      
      //COLORREF pixel = GetPixel(device_context, x, y);
      
      if (*pixel != 0) {
        if (min_x > x) min_x = x;
        if (max_x < x) max_x = x;
        
        if (min_y > y) min_y = y;
        if (max_y < y) max_y = y;
      }
      
      pixel++;
    }
    
    row -= MAX_FONT_WIDTH;
  }
  
  f32 kerning_change = 0.0f;
  if (min_x <= max_x) {
    
    i32 width  = (max_x - min_x) + 1;
    i32 height = (max_y - min_y) + 1;
    
    bmp.width = width + 2;
    bmp.height = height + 2;
    bmp.pitch = bmp.width * BITMAP_BYTES_PER_PIXEL;
    bmp.memory = malloc(bmp.height * bmp.pitch);
    bmp.free  = bmp.memory;
    
    memset(bmp.memory, 0, bmp.height * bmp.pitch);
    
    u8  *dst_row = (u8*)bmp.memory + (bmp.height - 1 - 1) * bmp.pitch;
    u32 *src_row = (u32*)global_font_bits + (MAX_FONT_HEIGHT - 1 - min_y) * MAX_FONT_WIDTH;
    
    for (i32 y = min_y; y <= max_y; y += 1) {
      u32 *src = (u32*)src_row + min_x;
      u32 *dst = (u32*)dst_row + 1;
      
      for (i32 x = min_x; x <= max_x; x += 1) {
        //COLORREF pixel = GetPixel(device_context, x, y);
        u32 pixel = *src;
        
        f32 gray = (f32)(pixel & 0xff);
        v4 texel = {255.0f, 255.0f, 255.0f, gray};
        texel = srgb255_to_linear1(texel);
        texel.rgb *= texel.a;
        texel = linear1_to_srgb255(texel);
        
        *dst++ = ((u32)(texel.a + .5f) << 24 | 
                  (u32)(texel.r + .5f) << 16 | 
                  (u32)(texel.g + .5f) << 8  |
                  (u32)(texel.b + .5f) << 0);
        
        src++;
      }
      
      dst_row -= bmp.pitch;
      src_row -= MAX_FONT_WIDTH;
    }
    
    asset->bitmap.align_pcent[0] = 1.0f / (f32)bmp.width;
    asset->bitmap.align_pcent[1] = (1.0f + (max_y - (bound_height - font->text_metric.tmDescent))) / (f32)bmp.height;
    
    kerning_change = (f32)(min_x - pre_step_x);
  }
  
#if 0
  ABC abc;
  GetCharABCWidthsW(global_font_device_context, code_point, code_point, &abc);
  f32 char_advance = (f32)(abc.abcA + abc.abcB + abc.abcC);
#else
  i32 this_width;
  GetCharWidth32W(global_font_device_context, code_point, code_point, &this_width);
  f32 char_advance = (f32)this_width;
#endif
  
  for (u32 other_glyph_index = 0; other_glyph_index < font->max_glyph_count; other_glyph_index++) {
    font->horizontal_advance[glyph_index * font->max_glyph_count + other_glyph_index] += char_advance - kerning_change;
    
    if (other_glyph_index != 0) {
      font->horizontal_advance[other_glyph_index * font->max_glyph_count + glyph_index] += kerning_change;
    }
    
  }
  
#else
  
  File_read_result font_file = read_entire_file(filename);
  assert(font_file.bytes_read);
  
  stbtt_fontinfo font;
  stbtt_InitFont(&font, (u8*)font_file.content, stbtt_GetFontOffsetForIndex((u8*)font_file.content, 0));
  
  f32 font_height = 128.0f;
  f32 scale_x = 0; // automatic
  f32 scale_y_px = stbtt_ScaleForPixelHeight(&font, font_height);
  i32 width, height, x_offset, y_offset;
  u8 *bitmap = stbtt_GetCodepointBitmap(&font, scale_x, scale_y_px, codepoint, &width, &height, &x_offset, &y_offset);
  
  bmp.width = width;
  bmp.height = height;
  bmp.pitch = bmp.width * BITMAP_BYTES_PER_PIXEL;
  bmp.memory = malloc(height * bmp.pitch);
  bmp.free  = bmp.memory;
  
  // Sean (stb_lib) top-down we bot-up
  u8 *src = bitmap;
  u8 *dst_row = (u8*)bmp.memory + (height - 1) * bmp.pitch;
  for (i32 y = 0; y < height; y += 1) {
    u32 *dst = (u32*)dst_row;
    for (i32 x = 0; x < width; x += 1) {
      
      u8 alpha = *src++;
      *dst++ = ((alpha) << 24 | 
                (alpha) << 16 | 
                (alpha) << 8  |
                (alpha) << 0);
    }
    dst_row -= bmp.pitch;
  }
  
  stbtt_FreeBitmap(bitmap, 0);
  free(font_file.content);
  
#endif
  
  return bmp;
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

struct Added_asset {
  u32 id;
  Hha_asset *hha;
  Asset_source *source;
};

static 
Added_asset
add_asset(Game_asset_list *asset_list) {
  assert(asset_list->debug_asset_type);
  assert(asset_list->debug_asset_type->one_past_last_asset_index < array_count(asset_list->asset_list));
  
  u32 index = asset_list->debug_asset_type->one_past_last_asset_index++;
  Asset_source *source = asset_list->asset_source_list + index;
  
  Hha_asset *hha = asset_list->asset_list + index;
  hha->first_tag_index = asset_list->tag_count;
  hha->one_past_last_tag_index = hha->first_tag_index;
  
  asset_list->asset_index = index;
  
  Added_asset result;
  result.id = index;
  result.hha = hha;
  result.source = source;
  
  return result;
}

static
Bitmap_id
add_bitmap_asset(Game_asset_list* asset_list, char *filename, f32 align_pcent_x = .5f, f32 align_pcent_y = .5f) {
  
  Added_asset asset = add_asset(asset_list);
  asset.hha->bitmap.align_pcent[0] = align_pcent_x;
  asset.hha->bitmap.align_pcent[1] = align_pcent_y;
  asset.source->type = Asset_type_bitmap;
  asset.source->bitmap.filename = filename;
  
  Bitmap_id result = {asset.id};
  return result;
}

static
Font_id
add_font_asset(Game_asset_list *asset_list, Loaded_font *font) {
  
  Added_asset asset = add_asset(asset_list);
  
  asset.hha->font.glyph_count = font->glyph_count;
  asset.hha->font.ascender_height  = (f32)font->text_metric.tmAscent;
  asset.hha->font.descender_height = (f32)font->text_metric.tmDescent;
  asset.hha->font.external_leading = (f32)font->text_metric.tmExternalLeading;
  asset.source->type = Asset_type_font;
  asset.source->font.font = font;
  
  Font_id result = { asset.id }; 
  return result;
}

static
Bitmap_id
add_char_asset(Game_asset_list *asset_list, Loaded_font *font, u32 code_point) {
  
  Added_asset asset = add_asset(asset_list);
  
  asset.hha->bitmap.align_pcent[0] = 0.0f;
  asset.hha->bitmap.align_pcent[1] = 0.0f;
  asset.source->type = Asset_type_font_glyph;
  asset.source->glyph.font = font;
  asset.source->glyph.code_point = code_point;
  
  Bitmap_id result = { asset.id };
  
  assert(font->glyph_count < font->max_glyph_count);
  u32 glyph_index = font->glyph_count++;
  Hha_font_glyph *glyph = font->glyph_list + glyph_index;
  glyph->unicode_code_point = code_point;
  glyph->bitmap_id = result;
  
  font->glyph_index_from_code_point[code_point] = glyph_index;
  
  return result;
}

static
Sound_id
add_sound_asset(Game_asset_list* asset_list, char *filename, u32 first_sample_index = 0, u32 sample_count = 0) {
  
  Added_asset asset = add_asset(asset_list);
  
  asset.hha->sound.sample_count = sample_count;
  asset.hha->sound.chain = Hha_sound_chain_none;
  
  asset.source->type = Asset_type_sound;
  asset.source->sound.filename = filename;
  asset.source->sound.first_sample_index = first_sample_index;
  
  Sound_id result = {asset.id};
  return result;
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
finalize_font_kerning(Loaded_font *font) {
  
  SelectObject(global_font_device_context, font->win32_handle);
  
  DWORD kerning_pair_count = GetKerningPairsW(global_font_device_context, 0, 0);
  auto kerning_pair_list = (KERNINGPAIR*)malloc(kerning_pair_count * sizeof(KERNINGPAIR));
  GetKerningPairsW(global_font_device_context, kerning_pair_count, kerning_pair_list);
  
  for (DWORD kerning_pair_index = 0; kerning_pair_index < kerning_pair_count; kerning_pair_index++) {
    KERNINGPAIR *pair = kerning_pair_list + kerning_pair_index;
    
    if (pair->wFirst  < ONE_PAST_MAX_FONT_CODEPOINT && 
        pair->wSecond < ONE_PAST_MAX_FONT_CODEPOINT) {
      u32 first  = font->glyph_index_from_code_point[pair->wFirst];
      u32 second = font->glyph_index_from_code_point[pair->wSecond];
      font->horizontal_advance[first * font->max_glyph_count + second] += (f32)pair->iKernAmount;
    }
  }
  
  free(kerning_pair_list);
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
        Loaded_sound wav = load_wav(src->sound.filename, src->sound.first_sample_index, dst->sound.sample_count);
        
        dst->sound.sample_count = wav.sample_count;
        dst->sound.channel_count = wav.channel_count;
        
        for (u32 channel_index = 0; channel_index < wav.channel_count; channel_index++) {
          size_t write_bytes = dst->sound.sample_count * sizeof(i16);
          fwrite(wav.samples[channel_index], write_bytes, _(count)1, out);
        }
        
        free(wav.free);
      }
      
      else if (src->type == Asset_type_font) {
        Loaded_font *font = src->font.font;
        
        finalize_font_kerning(font);
        
        u32 glyph_list_size = font->glyph_count * sizeof(Hha_font_glyph);
        fwrite(font->glyph_list, glyph_list_size, 1, out);
        
        u8 *horizontal_advance = (u8*)font->horizontal_advance;
        for (u32 glyph_index = 0; glyph_index < font->glyph_count; glyph_index++) {
          u32 horizontal_advance_slice_size = sizeof(f32) * font->glyph_count;
          
          fwrite(horizontal_advance, horizontal_advance_slice_size, 1, out);
          horizontal_advance += sizeof(f32) * font->max_glyph_count;
        }
        
      }
      
      else {
        Loaded_bmp bitmap;
        assert(src->type == Asset_type_bitmap || src->type == Asset_type_font_glyph);
        
        if (src->type == Asset_type_font_glyph) {
          bitmap = load_glyph(src->glyph.font, src->glyph.code_point, dst);
        }
        else {
          bitmap = load_bmp(src->bitmap.filename);
        }
        
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
Loaded_font*
load_ttf_font(char *filename, char *fontname) {
  
  Loaded_font *font = (Loaded_font*)malloc(sizeof(Loaded_font));
  
  AddFontResourceExA(filename, FR_PRIVATE, 0);
  i32 height = 128;
  
  assert(global_font_device_context);
  assert(global_font_bits);
  
  font->win32_handle = CreateFontA(height, 0, 0, 0,
                                   FW_NORMAL, // weight - normal, bold (0 to 1000) FW_NORMAL - 400
                                   _(italic)false,
                                   _(underline)false,
                                   _(strike out)false,
                                   DEFAULT_CHARSET, // ? ansi?
                                   OUT_DEFAULT_PRECIS, // precision
                                   CLIP_DEFAULT_PRECIS, // cliping
                                   ANTIALIASED_QUALITY, // anti-aliased type
                                   DEFAULT_PITCH|FF_DONTCARE, // pitch?
                                   fontname);
  
  SelectObject(global_font_device_context, font->win32_handle);
  GetTextMetrics(global_font_device_context, &font->text_metric);
  
  //font->min_code_point = INT_MAX;
  //font->max_code_point = 0;
  
  font->max_glyph_count = 5000;
  font->glyph_count = 0;
  
  u32 glyph_index_from_code_point_size = ONE_PAST_MAX_FONT_CODEPOINT * sizeof(Loaded_font);
  font->glyph_index_from_code_point = (u32*)malloc(glyph_index_from_code_point_size);
  memset(font->glyph_index_from_code_point, 0, glyph_index_from_code_point_size);
  
  font->glyph_list = (Hha_font_glyph*)malloc(sizeof(Hha_font_glyph) * font->max_glyph_count);
  size_t horizontal_advance_size = sizeof(f32) * font->max_glyph_count * font->max_glyph_count;
  font->horizontal_advance = (f32*)malloc(horizontal_advance_size);
  memset(font->horizontal_advance, 0, horizontal_advance_size);
  
  return font;
}

static
void
write_fonts() {
  
  Game_asset_list _asset_list_;
  Game_asset_list *asset_list = &_asset_list_;
  
  init(asset_list);
  
  Loaded_font *debug_font = load_ttf_font("c:/Windows/Fonts/arial.ttf", "Arial");
  //Loaded_font *debug_font = load_font("../data/DMSans_18pt-Regular.ttf", "DMSans", 256);
  
  begin_asset_type(asset_list, Asset_font_glyph);
  
  for (u32 ch = 0; ch <= 255; ch++) {
    add_char_asset(asset_list, debug_font, ch);
  }
  
  // Kanji
  add_char_asset(asset_list, debug_font, 0x5c0f);
  add_char_asset(asset_list, debug_font, 0x8033);
  add_char_asset(asset_list, debug_font, 0x6728);
  add_char_asset(asset_list, debug_font, 0x514e);
  
  end_asset_type(asset_list);
  
  begin_asset_type(asset_list, Asset_font);
  add_font_asset(asset_list, debug_font);
  end_asset_type(asset_list);
  
  write_hha_file(asset_list, "font_data.hha");
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
  
  write_hha_file(asset_list, "non_hero_data.hha");
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
  
  write_hha_file(asset_list, "hero_data.hha");
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
  
  write_hha_file(asset_list, "music_data.hha");
}

int
main(int arg_count, char **arg_list) {
  
  {
    global_font_device_context = CreateCompatibleDC(GetDC(0));
    
    BITMAPINFO info = {};
    
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = MAX_FONT_WIDTH;
    info.bmiHeader.biHeight = MAX_FONT_HEIGHT;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 0;
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;
    
    HBITMAP bitmap = CreateDIBSection(global_font_device_context, &info, DIB_RGB_COLORS, &global_font_bits, 0, 0);
    SelectObject(global_font_device_context, bitmap);
    SetBkColor(global_font_device_context, RGB(0, 0, 0));
  }
  write_fonts();
  
  write_non_hero();
  write_hero();
  write_sounds();
  
  return 1;
}