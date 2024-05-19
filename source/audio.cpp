
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

static
void
change_pitch(Audio_state *audio_state, Playing_sound *sound, f32 d_sample) {
  sound->d_sample = d_sample;
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
  playing_sound->current_volume = v2_one;
  playing_sound->target_volume = v2_one;
  playing_sound->d_current_volume = v2_zero;
  playing_sound->id = sound_id;
  playing_sound->d_sample = 1.0f;
  
  playing_sound->next = audio_state->first_playing_sound;
  audio_state->first_playing_sound = playing_sound;
  
  return playing_sound;
}

static
void
change_volume(Audio_state *audio_state, Playing_sound *sound, f32 fade_duration_sec, v2 volume)  {
  
  if (fade_duration_sec <= 0.0f) {
    sound->current_volume = sound->target_volume = volume;
  }
  else {
    f32 one_over_fade = 1.0f / fade_duration_sec;
    sound->target_volume = volume;
    sound->d_current_volume = one_over_fade * (sound->target_volume - sound->current_volume);
  }
  
}


#define f32x4 __m128  // simd 4x32 bytes floats?
#define i32x4 __m128i // simd 4x32 bytes ints OR 8x16 shorts

static
void
output_playing_sounds(Audio_state *audio_state, Game_sound_buffer *sound_buffer, Game_asset_list *asset_list, Memory_arena *temp_arena) {
  
  Temp_memory mixer_memory = begin_temp_memory(temp_arena);
  
  assert((sound_buffer->sample_count & 7) == 0); // aligned by 8
  u32 sample_count_8 = sound_buffer->sample_count / 8;
  u32 sample_count_4 = sound_buffer->sample_count / 4;
  
  f32x4 *real_channel_0 = mem_push_array(temp_arena, sample_count_4, f32x4, 16);
  f32x4 *real_channel_1 = mem_push_array(temp_arena, sample_count_4, f32x4, 16);
  
  f32 seconds_per_sample = 1.0f / sound_buffer->samples_per_second;
  
  f32x4 zero = _mm_set1_ps(0.0f);
  {
    f32x4 *dest_0 = real_channel_0;
    f32x4 *dest_1 = real_channel_1;
    
    for (u32 sample_index = 0; sample_index < sample_count_4; sample_index++) {
      _mm_store_ps((f32*)dest_0++, zero);
      _mm_store_ps((f32*)dest_1++, zero);
    }
  }
  
  for (Playing_sound **play_sound_ptr = &audio_state->first_playing_sound; *play_sound_ptr;) {
    
    Playing_sound *playing_sound = *play_sound_ptr;
    bool sound_finished = false;
    
    u32 total_samples_to_mix_8 = sample_count_8;
    f32x4 *dest_0 = real_channel_0;
    f32x4 *dest_1 = real_channel_1;
    
    while (total_samples_to_mix_8 && !sound_finished) {
      
      Loaded_sound *loaded_sound = get_sound(asset_list, playing_sound->id);
      
      if (loaded_sound) {
        Asset_sound_info *info = get_sound_info(asset_list, playing_sound->id);
        prefetch_sound(asset_list, info->next_id_to_play);
        
        v2 volume = playing_sound->current_volume;
        v2 d_volume   = seconds_per_sample * playing_sound->d_current_volume;
        v2 d_volume8  = d_volume * 8.0f;
        
        f32 d_sample  = playing_sound->d_sample;
        f32 d_sample8 = d_sample * 8.0f;
        
        f32x4 master_volume4_0 = _mm_set1_ps(audio_state->master_volume.e[0]);
        f32x4 master_volume4_1 = _mm_set1_ps(audio_state->master_volume.e[1]);
        
        f32x4 volume4_0 = _mm_setr_ps(volume.e[0] + 0.0f * d_volume.e[0],
                                      volume.e[0] + 1.0f * d_volume.e[0],
                                      volume.e[0] + 2.0f * d_volume.e[0],
                                      volume.e[0] + 3.0f * d_volume.e[0]);
        // set provided 32bit floats in reverse order
        
        f32x4 d_volume4_0 = _mm_set1_ps(d_volume.e[0]);
        f32x4 d_volume84_0 = _mm_set1_ps(d_volume8.e[0]);
        
        f32x4 volume4_1 = _mm_setr_ps(volume.e[1] + 0.0f * d_volume.e[1],
                                      volume.e[1] + 1.0f * d_volume.e[1],
                                      volume.e[1] + 2.0f * d_volume.e[1],
                                      volume.e[1] + 3.0f * d_volume.e[1]);
        // set provided 32bit floats in reverse order
        
        f32x4 d_volume4_1 = _mm_set1_ps(d_volume.e[1]);
        f32x4 d_volume84_1 = _mm_set1_ps(d_volume8.e[1]);
        
        assert(playing_sound->samples_played >= 0.0f);
        
        u32 samples_to_mix_8 = total_samples_to_mix_8;
        f32 real_sample_remaining_in_sound_8 = (loaded_sound->sample_count - round_f32_i32(playing_sound->samples_played)) / d_sample8;
        u32 samples_remaining_8 = round_f32_i32(real_sample_remaining_in_sound_8); 
        
        if (samples_to_mix_8 > samples_remaining_8) {
          samples_to_mix_8 = samples_remaining_8;
        }
        
        bool volume_ended[2] = {};
        for (u32 channel_index = 0; channel_index < array_count(volume_ended); channel_index++) {
          
          if (d_volume8.e[channel_index] != 0.0f) {
            f32 delta_volume = (playing_sound->target_volume.e[channel_index] - volume.e[channel_index]);
            
            u32 volume_sample_count_8 = (u32)((delta_volume / d_volume8.e[channel_index]) + 0.5f);
            
            if (samples_to_mix_8 > volume_sample_count_8) {
              samples_to_mix_8 = volume_sample_count_8;
              volume_ended[channel_index] = true;
            }
          }
          
        }
        
        f32 sample_pos = playing_sound->samples_played;
        for (u32 index = 0; index < samples_to_mix_8; index++) {
#if 0
          u32 sample_index = floor_f32_i32(sample_pos);
          
          f32 frac = sample_pos - (f32)sample_index;
          f32 sample_0 = (f32)loaded_sound->samples[0][sample_index];
          f32 sample_1 = (f32)loaded_sound->samples[0][sample_index + 1];
          f32 sample_value = lerp(sample_0, frac, sample_1);
#endif
          f32x4 sample_value_0 = _mm_setr_ps(loaded_sound->samples[0][round_f32_i32(sample_pos + 0.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 1.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 2.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 3.0f * d_sample)]);
          
          f32x4 sample_value_1 = _mm_setr_ps(loaded_sound->samples[0][round_f32_i32(sample_pos + 4.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 5.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 6.0f * d_sample)],
                                             loaded_sound->samples[0][round_f32_i32(sample_pos + 7.0f * d_sample)]);
          
          f32x4 d0_0 = _mm_load_ps((f32*)&dest_0[0]);
          f32x4 d0_1 = _mm_load_ps((f32*)&dest_0[1]);
          f32x4 d1_0 = _mm_load_ps((f32*)&dest_1[0]);
          f32x4 d1_1 = _mm_load_ps((f32*)&dest_1[1]);
          
          d0_0 = _mm_add_ps(d0_0, _mm_mul_ps(_mm_mul_ps(master_volume4_0, volume4_0), sample_value_0));
          d0_1 = _mm_add_ps(d0_1, _mm_mul_ps(_mm_mul_ps(master_volume4_0, _mm_add_ps(d_volume4_0, volume4_0)), sample_value_1));
          
          d1_0 = _mm_add_ps(d1_0, _mm_mul_ps(_mm_mul_ps(master_volume4_1, volume4_1), sample_value_0));
          d1_1 = _mm_add_ps(d1_1, _mm_mul_ps(_mm_mul_ps(master_volume4_1, _mm_add_ps(d_volume4_1, volume4_1)), sample_value_1));
          
          _mm_store_ps((f32*)&dest_0[0], d0_0);
          _mm_store_ps((f32*)&dest_0[1], d0_1);
          _mm_store_ps((f32*)&dest_1[0], d1_0);
          _mm_store_ps((f32*)&dest_1[1], d1_1);
          
          dest_0 += 2;
          dest_1 += 2;
          
          volume4_0 = _mm_add_ps(volume4_0, d_volume84_0);
          volume4_1 = _mm_add_ps(volume4_1, d_volume84_1);
          
          volume += d_volume8;
          sample_pos += d_sample8;
        }
        
        playing_sound->current_volume = volume;
        for (u32 channel_index = 0; channel_index < array_count(volume_ended); channel_index++) {
          if (volume_ended[channel_index]) {
            playing_sound->current_volume.e[channel_index] = playing_sound->target_volume.e[channel_index];
            
            playing_sound->d_current_volume.e[channel_index] = 0.0f;
          }
        }
        
        playing_sound->samples_played = sample_pos;
        assert(total_samples_to_mix_8 >= samples_to_mix_8);
        total_samples_to_mix_8 -= samples_to_mix_8;
        
        if ((u32)playing_sound->samples_played >= loaded_sound->sample_count) {
          if (is_valid(info->next_id_to_play)) {
            playing_sound->id = info->next_id_to_play;
            playing_sound->samples_played = 0;
          }
          else {
            sound_finished = true;
          }
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
    f32x4 *source_0 = real_channel_0;
    f32x4 *source_1 = real_channel_1;
    
    i32x4 *sample_out = (i32x4*)sound_buffer->samples;
    
    for (u32 sample_index = 0; sample_index < sample_count_4; sample_index++) {
      
      f32x4 s0 = _mm_load_ps((f32*)source_0++); // load 4 floats 
      f32x4 s1 = _mm_load_ps((f32*)source_1++); 
      
      i32x4 L = _mm_cvtps_epi32(s0);              // convert them to ints
      i32x4 R = _mm_cvtps_epi32(s1);
      
      i32x4 lr0 = _mm_unpacklo_epi32(L, R);       // from L and R take low  half
      i32x4 lr1 = _mm_unpackhi_epi32(L, R);       // from L and R take high half
      // it will pack interleaved so it goes lr0_0, lr0_1
      // so we have packed sample from left and right interleaved
      
      i32x4 sample = _mm_packs_epi32(lr0, lr1);   // convert 32 ints from lr0,lr1 to 16-bit
      
      *sample_out++ = sample;
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
  audio_state->master_volume = v2_one;
}