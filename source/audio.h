/* date = May 9th 2024 0:48 pm */

#ifndef AUDIO_H
#define AUDIO_H

struct Playing_sound {
  f32 volume[2];
  Sound_id id;
  u32 samples_played;
  Playing_sound *next;
};

struct Audio_state {
  Memory_arena *perm_arena;
  Playing_sound *first_playing_sound;
  Playing_sound *first_free_playing_sound;
};

#endif //AUDIO_H
