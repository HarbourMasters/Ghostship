#pragma once

#include "port/hooks/impl/EventSystem.h"
#include "engine/graph_node.h"

typedef s32 (*LevelScriptFunction)(s16, s32);

DEFINE_EVENT(GameFrameUpdate);

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

DEFINE_EVENT(LevelScriptExecute,
    u8 command;
);