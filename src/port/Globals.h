#pragma once

#include <vector>
#include <Context.h>

class GameEngine {
public:
    static GameEngine* Instance;

    std::shared_ptr<LUS::Context> context;

    GameEngine();
    ~GameEngine();
    void run();
};

void GameEngine_Create(void);
void GameEngine_StartFrame(void);
void GameEngine_RunCommands(Gfx* Commands);
void GameEngine_ProcessFrame(void (*run_one_game_iter)(void));