#include <libultraship.h>
#include <fast/interpreter.h>

#include "sm64.h"
#include "port/ShipInit.hpp"
#include "port/events/Events.h"
#include "game/area.h"
#include "game/camera.h"
#include "game/level_update.h"
#include "game/mario.h"

enum UniformSlot {
    LEVEL_STATE_SLOT = 2,
    MARIO_STATE_SLOT = 3,
    MARIO_POS_SLOT = 4,
    CAMERA_POS_SLOT = 5,
    FLAGS_SLOT = 6,
    WATER_SLOT = 7,
};

static void OnGameLoopTick(IEvent* event) {
    // Slot 2: Level/Area state (level, area, act, courseNum)
    float levelData[4] = {
        static_cast<float>(gCurrLevelNum),
        static_cast<float>(gCurrAreaIndex),
        static_cast<float>(gCurrActNum),
        static_cast<float>(gCurrCourseNum)
    };
    gfx_set_custom_uniform(LEVEL_STATE_SLOT, levelData);

    // Slot 3: Mario state (health 0-1, lives, coins, capTimer 0-1)
    float marioStateData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (gMarioState) {
        marioStateData[0] = static_cast<float>(gMarioState->health & 0xFF) / 8.0f;
        marioStateData[1] = static_cast<float>(gMarioState->numLives);
        marioStateData[2] = static_cast<float>(gMarioState->numCoins);
        marioStateData[3] = static_cast<float>(gMarioState->capTimer) / 1200.0f;
    }
    gfx_set_custom_uniform(MARIO_STATE_SLOT, marioStateData);

    // Slot 4: Mario position (pos X, Y, Z, forwardVel)
    float marioPosData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (gMarioState) {
        marioPosData[0] = gMarioState->pos[0];
        marioPosData[1] = gMarioState->pos[1];
        marioPosData[2] = gMarioState->pos[2];
        marioPosData[3] = gMarioState->forwardVel;
    }
    gfx_set_custom_uniform(MARIO_POS_SLOT, marioPosData);

    // Slot 5: Camera position (cam X, Y, Z, unused)
    float cameraPosData[4] = {
        gLakituState.curPos[0],
        gLakituState.curPos[1],
        gLakituState.curPos[2],
        0.0f
    };
    gfx_set_custom_uniform(CAMERA_POS_SLOT, cameraPosData);

    // Slot 6: Flags (mario flags, action, terrainType, cameraMode)
    float flagsData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (gMarioState) {
        flagsData[0] = static_cast<float>(gMarioState->flags);
        flagsData[1] = static_cast<float>(gMarioState->action);
    }
    if (gCurrentArea) {
        flagsData[2] = static_cast<float>(gCurrentArea->terrainType);
    }
    if (gCamera) {
        flagsData[3] = static_cast<float>(gCamera->mode);
    }
    gfx_set_custom_uniform(FLAGS_SLOT, flagsData);

    // Slot 7: Water state (waterLevel Y, isUnderwater, isInTransition, particleFlags)
    float waterData[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (gMarioState) {
        waterData[0] = static_cast<float>(gMarioState->waterLevel);
        waterData[1] = (gMarioState->action & ACT_FLAG_SWIMMING) ? 1.0f : 0.0f;
        waterData[3] = static_cast<float>(gMarioState->particleFlags);
    }
    waterData[2] = (gWarpTransition.isActive) ? 1.0f : 0.0f;
    gfx_set_custom_uniform(WATER_SLOT, waterData);
}

static void Init() {
    REGISTER_LISTENER(GameLoopTick, EVENT_PRIORITY_NORMAL, OnGameLoopTick);
}

static RegisterShipInitFunc initFunc(Init);

