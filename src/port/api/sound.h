#pragma once

#include <stdint.h>

#include "api.h"

// Sounds and music of a mod's own, from WAV, MP3 or FLAC files in its archive. They are decoded when registered and
// mixed into the game's output, following the master, main music and sound effect volume settings.

// Registers a sound from a file in a mod archive, e.g. "sound/chime.wav". Returns its number, or -1.
GS_API int C_RegisterSound(const char* path);
// Same as C_RegisterSound, from file data in memory, which is copied.
GS_API int C_RegisterSoundData(const uint8_t* data, uint32_t size);
// Unregisters a sound and stops it.
GS_API void C_UnregisterSound(int sound);
// Length of a sound in samples at 32 kHz, 0 for no sound.
GS_API uint32_t C_SoundLength(int sound);

// Plays a sound effect at volume 0 to 1 and pan -1 (left) to 1 (right). Returns a voice, or -1.
GS_API int C_PlaySound(int sound, float volume, float pan);
GS_API void C_StopSound(int voice);
GS_API int C_IsSoundPlaying(int voice);

// Plays a sound as music, silencing the game's level music until C_StopMusic. loop_start is the sample (at 32 kHz)
// the music goes back to when it ends, -1 to play it once.
GS_API void C_PlayMusic(int sound, int32_t loop_start);
GS_API void C_StopMusic(void);
// Plays the sound whenever the game starts level music seq_id (SEQ_* in seq_ids.h), until the game stops, fades out
// or changes it. sound -1 gives the game's own music back.
GS_API void C_ReplaceMusic(uint8_t seq_id, int sound, int32_t loop_start);
