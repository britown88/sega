#pragma once

#include "segautils\Coroutine.h"
#include "Entities\Entities.h"
#include "Actions.h"

typedef struct WorldView_t WorldView;

Coroutine createCommandGridMove(Action *a, WorldView *view);