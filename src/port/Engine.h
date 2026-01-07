#pragma once

#define LOAD_ASSET(path) (path == NULL ? NULL : (GameEngine_OTRSigCheck((const char*) path) ? ResourceGetDataByName((const char*) path) : path))
#define LOAD_ASSET_RAW(path) ResourceGetDataByName((const char*) path)

#ifdef __cplusplus
#include <vector>
#include <ship/controller/controldeck/ControlDeck.h>
#include <ship/Context.h>
#include <fast/interpreter.h>

#ifndef IDYES
#define IDYES 6
#endif
#ifndef IDNO
#define IDNO 7
#endif

#define SAMPLES_HIGH 544
#define SAMPLES_LOW 528
#define AUDIO_FRAMES_PER_UPDATE 2
#define NUM_AUDIO_CHANNELS 2
#define SAMPLES_PER_FRAME (SAMPLES_HIGH * NUM_AUDIO_CHANNELS * 2)

struct Animation;
struct CtlEntry;
struct AudioBankSample;
struct AudioSequenceData;

class GameEngine {
  public:
    static GameEngine* Instance;

    std::shared_ptr<Ship::Context> context;
    std::vector<Animation*> animationsTable;

    std::vector<CtlEntry*> banksTable;
    std::vector<std::string> sequenceTable;
    std::vector<AudioSequenceData*> audioSequenceTable;

    std::unordered_map<std::string, uint8_t> bankMapTable;
    std::unordered_map<std::string, std::vector<uint8_t>>* dictionary;

    GameEngine();
    static void Create();
    static bool GenAssetFile(bool exitOnFail = true);
    void AudioInit();
    void StartFrame() const;
    static void RunCommands(Gfx* Commands, const std::vector<std::unordered_map<Mtx*, MtxF>>& mtx_replacements);
    static uint32_t GetInterpolationFPS();
    static void HandleAudioThread();
    static void StartAudioFrame();
    static void EndAudioFrame();
    static void AudioExit();
    static void ProcessGfxCommands(Gfx* commands);
    static uint8_t GetBankIdByName(const std::string& name);
    static int ShowYesNoBox(const char* title, const char* box);
    static void ShowMessage(const char* title, const char* message, SDL_MessageBoxFlags type = SDL_MESSAGEBOX_ERROR);
    void LoadDictionary();
    void LoadPlayerAnims();
    static uint32_t GetGameVersion();
    static void Destroy();
};

Fast::Interpreter* GameEngine_GetInterpreter();

extern "C" {
#endif
void GameEngine_ProcessGfxCommands(Gfx* commands);
uint32_t GameEngine_GetInterpolatedFPS();
uint32_t GameEngine_GetSampleRate();
uint32_t GameEngine_GetSamplesPerFrame();
float GameEngine_GetAspectRatio();
struct CtlEntry* GameEngine_LoadBank(uint8_t bankId);
uint8_t GameEngine_IsBankLoaded(uint8_t bankId);
void GameEngine_UnloadBank(uint8_t bankId);
struct AudioSequenceData* GameEngine_LoadSequence(uint8_t seqId);
uint32_t GameEngine_GetSequenceCount();
uint8_t GameEngine_IsSequenceLoaded(uint8_t seqId);
void GameEngine_UnloadSequence(uint8_t seqId);
uint32_t GameEngine_GetGameVersion();
uint8_t* GameEngine_LoadActName(uint32_t actId);
uint8_t* GameEngine_LoadLevelName(uint32_t levelId);
struct DialogEntry* GameEngine_LoadDialog(uint32_t dialogId);
uint8_t* GameEngine_LoadTranslation(const char* key);
int GameEngine_OTRSigCheck(const char* imgData);
struct Animation* GameEngine_LoadAnimation(uint32_t animId);
#ifdef __cplusplus
}
#endif