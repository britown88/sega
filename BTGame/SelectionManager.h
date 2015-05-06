#pragma once

#include "Entities\Entities.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

typedef struct EntitySystem_t EntitySystem;
typedef struct SelectionManager_t SelectionManager;

typedef enum {
   scTeam,
   scArea
} SelectCriteriaType;

typedef struct {
   SelectCriteriaType type;

   union {
      Recti box;
      size_t teamID;
   };
}SelectCriteria;

SelectionManager *createSelectionManager(EntitySystem *system);
void selectionManagerSelectEx(SelectionManager *self, SelectCriteria *filters, size_t filterCount);
vec(EntityPtr) *selectionManagerGetSelected(SelectionManager *self);

#define selectionManagerSelect(__selManager, ...) {\
      SelectCriteria __filerList[] = { __VA_ARGS__ }; \
      selectionManagerSelectEx(__selManager, __filerList, sizeof(__filerList)/sizeof(__filerList[0])); \
   }

#define selectionManagerSelectAll(__selManager) {\
      selectionManagerSelectEx(__selManager, NULL, 0); \
   }