#pragma once

#include "segautils\Coroutine.h"
#include "Entities\Entities.h"
#include "Actions.h"

typedef struct GridManager_t GridManager;

Coroutine createCommandGridMove(Action *a, GridManager *manager, bool setPrimary);