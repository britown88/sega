#pragma once

#include "SEGA\App.h"
#include "segautils\DLLBullshit.h"

DLL_PUBLIC VirtualApp *btCreate();

typedef struct EntitySystem_t EntitySystem;
typedef struct GridManager_t GridManager;

void derjpkstras(EntitySystem *system, GridManager *manager);