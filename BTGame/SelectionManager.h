#pragma once

#include "Entities\Entities.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

typedef struct EntitySystem_t EntitySystem;
typedef struct SelectionManager_t SelectionManager;

#define ClosureTPart \
    CLOSURE_RET(bool) /*return whether or not to select*/\
    CLOSURE_NAME(SelectionCriteria) \
    CLOSURE_ARGS(EntitySystem *, Entity *)
#include "segautils\Closure_Decl.h"

typedef struct {
   Recti box;
   size_t teamID;
}SelectDataAreaTeam;
SelectionCriteria selectAreaByTeam(SelectDataAreaTeam *data);

SelectionManager *createSelectionManager(EntitySystem *system);
void selectionManagerSelect(SelectionManager *self, SelectionCriteria criteria);