
internal
void
output_test_sine_wave(Game_state* state, Game_sound_buffer* sound_buffer, i32 tone_hz) {
  i16 tone_volume = 1000;
  i32 wave_period = sound_buffer->samples_per_second / tone_hz;
  auto sample_out = sound_buffer->samples;
  
#if 0
  for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
    i16 sample_value = 0;
    
    *sample_out++ = sample_value;
    *sample_out++ = sample_value;
  }
#else
  for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
    f32 sine_val = sin(state->t_sine);
    
    f32 modulation_depth = 100.0f; 
    f32 base_frequency = 100.0f; 
    f32 mod_freq = sin(state->t_mod) * modulation_depth + base_frequency;
    f32 phase_increment = PI * 2.0f * mod_freq / (f32)wave_period;
    i16 sample_value = (i16)(sine_val * tone_volume);
    
    *sample_out++ = sample_value;
    *sample_out++ = sample_value;
    
    state->t_sine += phase_increment;
    state->t_mod += 0.1f;
    
    if (state->t_sine > TAU)
      state->t_sine -= TAU;
    
    if (state->t_mod > TAU)
      state->t_mod -= TAU;
  }
#endif
}


internal
Playing_sound*
play_sound(Audio_state *audio_state, Sound_id sound_id) {
  
  if (!audio_state->first_free_playing_sound) {
    audio_state->first_free_playing_sound = mem_push_struct(audio_state->perm_arena, Playing_sound);
    audio_state->first_free_playing_sound->next = 0;
  }
  
  Playing_sound *playing_sound = audio_state->first_free_playing_sound;
  audio_state->first_free_playing_sound = playing_sound->next;
  
  playing_sound->samples_played = 0;
  playing_sound->volume[0] = 1.0f;
  playing_sound->volume[1] = 1.0f;
  playing_sound->id = sound_id;
  
  playing_sound->next = audio_state->first_playing_sound;
  audio_state->first_playing_sound = playing_sound;
  
  return playing_sound;
}

static
void
output_playing_sounds(Audio_state *audio_state, Game_sound_buffer *sound_buffer, Game_asset_list *asset_list, Memory_arena *temp_arena) {
  
  Temp_memory mixer_memory = begin_temp_memory(temp_arena);
  
  f32 *real_channel_0 = mem_push_array(temp_arena, sound_buffer->sample_count, f32);
  f32 *real_channel_1 = mem_push_array(temp_arena, sound_buffer->sample_count, f32);
  
  {
    f32 *dest_0 = real_channel_0;
    f32 *dest_1 = real_channel_1;
    
    for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
      *dest_0++ = 0.0f;
      *dest_1++ = 0.0f;
    }
  }
  
  for (Playing_sound **play_sound_ptr = &audio_state->first_playing_sound; *play_sound_ptr;) {
    
    Playing_sound *playing_sound = *play_sound_ptr;
    bool sound_finished = false;
    
    u32 total_samples_to_mix = sound_buffer->sample_count;
    f32 *dest_0 = real_channel_0;
    f32 *dest_1 = real_channel_1;
    
    while (total_samples_to_mix && !sound_finished) {
      
      Loaded_sound *loaded_sound = get_sound(asset_list, playing_sound->id);
      
      if (loaded_sound) {
        
        Asset_sound_info *info = get_sound_info(asset_list, playing_sound->id);
        prefetch_sound(asset_list, info->next_id_to_play);
        
        f32 volume_0 = playing_sound->volume[0];
        f32 volume_1 = playing_sound->volume[1];
        
        assert(playing_sound->samples_played >= 0);
        
        u32 samples_to_mix = sound_buffer->sample_count;
        u32 samples_remaining = loaded_sound->sample_count - playing_sound->samples_played;
        
        if (samples_to_mix > samples_remaining) {
          samples_to_mix = samples_remaining;
        }
        
        for (u32 sample_index = playing_sound->samples_played; sample_index < playing_sound->samples_played + samples_to_mix; sample_index++) {
          f32 sample_value = loaded_sound->samples[0][sample_index];
          *dest_0++ += volume_0 * sample_value;
          *dest_1++ += volume_1 * sample_value;
        }
        
        assert(total_samples_to_mix >= samples_to_mix);
        playing_sound->samples_played += samples_to_mix;
        total_samples_to_mix -= samples_to_mix;
        
        if ((u32)playing_sound->samples_played == loaded_sound->sample_count) {
          if (is_valid(info->next_id_to_play)) {
            playing_sound->id = info->next_id_to_play;
            playing_sound->samples_played = 0;
          }
          else {
            sound_finished = true;
          }
        }
        else {
          assert(total_samples_to_mix == 0);
        }
        
      }
      else {
        load_sound(asset_list, playing_sound->id);
        break;
      }
    }
    
    if (sound_finished) {
      *play_sound_ptr = playing_sound->next;
      playing_sound->next = audio_state->first_free_playing_sound;
      audio_state->first_free_playing_sound = playing_sound;
    }
    else {
      play_sound_ptr = &playing_sound->next;
    }
  }
  
  {
    f32 *source_0 = real_channel_0;
    f32 *source_1 = real_channel_1;
    
    i16 *sample_out = sound_buffer->samples;
    for (i32 sample_index = 0; sample_index < sound_buffer->sample_count; sample_index++) {
      
      *sample_out++ = (i16)(*source_0++ + 0.5f);
      *sample_out++ = (i16)(*source_1++ + 0.5f);
    }
  }
  
  end_temp_memory(mixer_memory);
}

static
void
initialize_audio_state(Audio_state *audio_state, Memory_arena *arena) {
  audio_state->perm_arena = arena;
  audio_state->first_playing_sound = 0;
  audio_state->first_free_playing_sound = 0;
}