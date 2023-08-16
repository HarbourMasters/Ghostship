#pragma once

#ifdef __cplusplus
#include <vector>
#include <Context.h>

#define SAMPLES_HIGH 752
#define SAMPLES_LOW 720
#define AUDIO_FRAMES_PER_UPDATE 2
#define NUM_AUDIO_CHANNELS 2

class GameEngine {
  public:
    static GameEngine* Instance;

    std::shared_ptr<LUS::Context> context;

    GameEngine();
    ~GameEngine();
    static void Create(void);
    void StartFrame(void);
    void RunCommands(Gfx* Commands);
    void ProcessFrame(void (*run_one_game_iter)(void));
};
#else
uint32_t GameEngine_GetSampleRate(void);
#endif