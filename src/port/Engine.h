#pragma once

#ifdef __cplusplus
#include <vector>
#include <Context.h>

#define SAMPLES_HIGH 544
#define SAMPLES_LOW 528
#define AUDIO_FRAMES_PER_UPDATE 2
#define NUM_AUDIO_CHANNELS 2

struct CtlEntry;
struct AudioBankSample;
struct AudioSequenceData;

class GameEngine {
  public:
    static GameEngine* Instance;

    std::shared_ptr<LUS::Context> context;
    std::unordered_map<uint8_t, CtlEntry*> banks;
    std::unordered_map<uint8_t, AudioSequenceData*> sequences;

    GameEngine();
    static void Create();
    void StartFrame() const;
    static void RunCommands(Gfx* Commands);
    void ProcessFrame(void (*run_one_game_iter)()) const;
    static void HandleAudioThread();
    static void StartAudioFrame();
    static void EndAudioFrame();
    static void AudioInit();
    static void AudioExit();

    void Destroy();
};
#else

uint32_t GameEngine_GetSampleRate();
float GameEngine_GetAspectRatio();
struct CtlEntry* GameEngine_LoadBank(uint8_t bankId);
uint8_t GameEngine_IsBankLoaded(uint8_t bankId);
void GameEngine_UnloadBank(uint8_t bankId);
struct AudioSequenceData* GameEngine_LoadSequence(uint8_t seqId);
uint8_t GameEngine_IsSequenceLoaded(uint8_t seqId);
void GameEngine_UnloadSequence(uint8_t seqId);
uint32_t GameEngine_GetGameVersion();
uint8_t* GameEngine_LoadActName(uint32_t actId);
uint8_t* GameEngine_LoadLevelName(uint32_t levelId);
struct DialogEntry* GameEngine_LoadDialog(uint32_t dialogId);
#endif