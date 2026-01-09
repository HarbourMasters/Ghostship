#pragma once

#include "port/hooks/impl/EventSystem.h"

DEFINE_EVENT(GameFrameUpdate);

DEFINE_EVENT(RenderPauseCourseOptions,
    bool* render;
);