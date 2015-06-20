#pragma once

#include "GridManager.h"
#include "Entities\Entities.h"

Entity *gridFindClosestEntity(GridManager *manager, size_t start, size_t enemyTeamID, float maxRange);
