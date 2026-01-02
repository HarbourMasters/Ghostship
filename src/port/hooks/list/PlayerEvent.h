#pragma once

#include "port/hooks/impl/EventSystem.h"

DEFINE_EVENT(PlayerHealthChange, 
    int16_t health;
);