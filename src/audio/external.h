#ifndef AUDIO_EXTERNAL_H
#define AUDIO_EXTERNAL_H

#include <libultra/types.h>

#include "types.h"

// Sequence arguments, passed to seq_player_play_sequence. seqId may be bit-OR'ed with
// SEQ_VARIATION; this will load the same sequence, but set a variation
// bit which may be read by the sequence script.
#define SEQUENCE_ARGS(priority, seqId) ((priority << 8) | seqId)

#define SOUND_MODE_STEREO           0
#define SOUND_MODE_MONO             3
#define SOUND_MODE_HEADSET          1
#define SOUND_MODE_SURROUND         2

#define SEQ_PLAYER_LEVEL            0  // Level background music
#define SEQ_PLAYER_ENV              1  // Misc music like the puzzle jingle
#define SEQ_PLAYER_SFX              2  // Sound effects


extern_s s32 gAudioErrorFlags;
extern_s f32 gGlobalSoundSource[3];

// defined in data.c, used by the game
extern_s u32 gAudioRandom;

extern_s u8 gAudioSPTaskYieldBuffer[]; // ucode yield data ptr; only used in JP

extern_s struct SPTask *create_next_audio_frame_task(void);
extern_s void play_sound(s32 soundBits, f32 *pos);
extern_s void audio_signal_game_loop_tick(void);
extern_s void seq_player_fade_out(u8 player, u16 fadeDuration);
extern_s void fade_volume_scale(u8 player, u8 targetScale, u16 fadeDuration);
extern_s void seq_player_lower_volume(u8 player, u16 fadeDuration, u8 percentage);
extern_s void seq_player_unlower_volume(u8 player, u16 fadeDuration);
extern_s void set_audio_muted(u8 muted);
extern_s void sound_init(void);
extern_s void get_currently_playing_sound(u8 bank, u8 *numPlayingSounds, u8 *numSoundsInBank, u8 *soundId);
extern_s void stop_sound(u32 soundBits, f32 *pos);
extern_s void stop_sounds_from_source(f32 *pos);
extern_s void stop_sounds_in_continuous_banks(void);
extern_s void sound_banks_disable(u8 player, u16 bankMask);
extern_s void sound_banks_enable(u8 player, u16 bankMask);
extern_s void set_sound_moving_speed(u8 bank, u8 speed);
extern_s void play_dialog_sound(u8 dialogID);
extern_s u8 is_sequence_playing(u16 seqId);
extern_s void play_music(u8 player, u16 seqArgs, u16 fadeTimer);
extern_s void stop_background_music(u16 seqId);
extern_s void fadeout_background_music(u16 arg0, u16 fadeOut);
extern_s void drop_queued_background_music(void);
extern_s u16 get_current_background_music(void);
extern_s void play_secondary_music(u8 seqId, u8 bgMusicVolume, u8 volume, u16 fadeTimer);
extern_s void func_80321080(u16 fadeTimer);
extern_s void func_803210D4(u16 fadeOutTime);
extern_s void play_course_clear(void);
extern_s void play_peachs_jingle(void);
extern_s void play_puzzle_jingle(void);
extern_s void play_star_fanfare(void);
extern_s void play_power_star_jingle(u8 arg0);
extern_s void play_race_fanfare(void);
extern_s void play_toads_jingle(void);
extern_s void sound_reset(u8 presetId);
extern_s void audio_set_sound_mode(u8 arg0);
extern_s void audio_set_player_volume(u8 player, f32 volume);
extern_s void create_next_audio_buffer(s16 *samples, u32 num_samples);
extern_s void audio_init(void); // in load.c

#if defined(VERSION_EU) || defined(VERSION_SH)
struct SPTask *unused_80321460();
struct SPTask *unused_80321460(void);
#endif

#endif // AUDIO_EXTERNAL_H
