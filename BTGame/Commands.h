#pragma once

#include "segautils\Coroutine.h"
#include "Entities\Entities.h"

typedef struct GridManager_t GridManager;

Coroutine createCommandGridMove(Entity *e, GridManager *manager, int x, int y);