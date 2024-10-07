/* date = February 12th 2024 1:52 pm */

#ifndef ASSET_H
#define ASSET_H

#include "asset_type_id.h"

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

// https://docs.fileformat.com/audio/wav/
// https://onestepcode.com/modifying-wav-files-c/
struct Wave_fmt_ {
  u16 riff_header; // "RIFF"
  u32 wav_size;    // file size?
  u16 wave_header; // "WAVE"
  u16 fmt_header;  // "fmt "
  u32 fmt_chunk_size; // 16
  
  u16 audio_format; // 1 - PCM?
  u16 channel_count; // 2
  u32 sample_rate; // 44100 samples per second/ hertz
  u32 byte_rate;   // (Sample Rate * BitsPerSample * Channels) / 8 // 176400
  u16 block_align; // (BitsPerSample * Channels) / 8.1 - 8 bit mono2 - 8 bit stereo/16 bit mono4 - 16 bit stereo
  u16 bits_per_sample; // 16
  u16 data_header; // "data"
  u32 data_size;  // size of data (samples)
};

#pragma pack(pop)

struct Bitmap_id {
  u32 value;
};

struct Sound_id {
  u32 value;
};

struct Loaded_sound {
  u32 sample_count;
  u32 channel_count;
  i16 *samples[2];
};

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
  union {
    Loaded_bmp *bitmap;
    Loaded_sound *sound;
  };
};

struct Asset_tag {
  u32 id;
  f32 value;
};

struct Asset_vector {
  f32 e[Tag_count];
};

struct Asset_type {
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

struct Asset_bitmap_info {
  char *filename;
  v2 align_pcent;
};

struct Asset_sound_info {
  char *filename;
  u32 first_sample_index;
  u32 sample_count;
  Sound_id next_id_to_play;
};

struct Asset {
  u32 first_tag_index;
  u32 one_past_last_tag_index;
  
  union {
    Asset_bitmap_info bitmap;
    Asset_sound_info  sound;
  };
};

struct Game_asset_list {
  
  struct Transient_state *tran_state;
  Memory_arena arena;
  
  f32 tag_range[Tag_count];
  
  u32 tag_count;
  Asset_tag *tag_list;
  
  u32 asset_count;
  Asset *asset_list;
  Asset_slot *slot_list;
  
  Asset_type asset_type_list[Asset_count];
  
  //Hero_bitmaps hero_bitmaps[4];
  
#if  0
  u32 debug_used_asset_count;
  u32 debug_used_tag_count;
  
  Asset_type *debug_asset_type;
  Asset *debug_asset;
#endif
};

void load_bitmap(Game_asset_list *asset_list, Bitmap_id id);
void load_sound(Game_asset_list *asset_list, Sound_id id);

inline
void 
prefetch_bitmap(Game_asset_list *asset_list, Bitmap_id id) { 
  load_bitmap(asset_list, id);
}

inline
void 
prefetch_sound(Game_asset_list *asset_list, Sound_id id) { 
  load_sound(asset_list, id);
}

#endif //ASSET_H
