#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Sounds from mods, mixed into the game's 32 kHz stereo output after the game's own audio.
namespace ModSound {

// Decodes a WAV, MP3 or FLAC file into 32 kHz stereo samples. Returns false when it is none of them.
bool Decode(const uint8_t* data, size_t size, std::vector<int16_t>& out);

// Adds decoded samples and returns their sound number.
int Add(std::vector<int16_t> samples);
// Removes a sound and stops the voices playing it.
void Remove(int sound);
// Number of stereo frames in a sound, 0 when there is no such sound.
size_t Length(int sound);

// Starts a voice and returns its number, or -1. volume is 0 to 1, pan -1 (left) to 1 (right).
int Play(int sound, float volume, float pan);
void Stop(int voice);
bool Playing(int voice);

// Plays a sound as music, replacing the music playing before. loopStart is the frame the music goes back to at its
// end, -1 to play it once. The game's music is silent while it plays.
void PlayMusic(int sound, int loopStart);
void StopMusic();
bool MusicActive();

// Adds the playing voices to an interleaved stereo frame.
void Mix(int16_t* stereo, int frames, float musicVolume, float sfxVolume);

} // namespace ModSound
