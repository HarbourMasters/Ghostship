#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t ModAudioHandle;
#define MOD_AUDIO_INVALID 0

// Register a mod audio source. Returns a handle on success, MOD_AUDIO_INVALID on failure.
ModAudioHandle ModAudio_Register(void);
// Unregister and free the audio source.
void ModAudio_Unregister(ModAudioHandle handle);
// Submit interleaved stereo s16 samples at engine sample rate (32000 Hz).
void ModAudio_SubmitSamples(ModAudioHandle handle, const int16_t* samples, uint32_t stereo_count);
// Returns the engine sample rate (32000 Hz).
uint32_t ModAudio_GetSampleRate(void);
// Returns stereo sample pairs per engine audio frame.
uint32_t ModAudio_GetFrameSize(void);
// Returns how many stereo pairs are currently buffered for this source.
uint32_t ModAudio_GetFillLevel(ModAudioHandle handle);

#ifdef __cplusplus
}

void ModAudio_MixInto(int16_t* buf, uint32_t stereo_count);
#endif
