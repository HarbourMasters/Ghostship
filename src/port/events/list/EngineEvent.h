#pragma once

#include "engine/graph_node.h"

typedef s32 (*LevelScriptFunction)(s16, s32);

DEFINE_EVENT(GameFrameUpdate);
DEFINE_EVENT(GameLoopTick);
DEFINE_EVENT(GameReadInput);

DEFINE_EVENT(RenderHud);
DEFINE_EVENT(RenderTextLabels);
DEFINE_EVENT(RenderGamePre);
DEFINE_EVENT(RenderGamePost);

DEFINE_EVENT(RenderPauseCourseOptions,
    bool* render;
);

DEFINE_EVENT(LevelScriptCallLoop,
    LevelScriptFunction* func;
    int16_t* arg;
)

DEFINE_EVENT(LevelScriptBeginArea,
    uint8_t* areaIndex;
    void** geoLayoutAddr;
);

DEFINE_EVENT(GeoLayoutCallASM, 
    GraphNodeFunc* func;
    int16_t* parameter;
);

DEFINE_EVENT(LevelInitFromSaveFile);

DEFINE_EVENT(OnGameFileLoad,
    s32 fileNum;
);

DEFINE_EVENT(OnGameFileSave,
    s32 fileNum;
);

DEFINE_EVENT(LevelScriptExecute,
    u8 command;
);

// TODO: These could probably have better names, but for now they are fine.
DEFINE_EVENT(EntityDistanceRender,
    bool* visible;
);

DEFINE_EVENT(EntityDistanceLoad,
    bool* visible;
);

DEFINE_EVENT(BehaviorCallNative,
    void (*function)(void);
);

DEFINE_EVENT(EngineReady);

DEFINE_EVENT(CameraUpdate,
    struct Camera* c;
);