#pragma once

#include "Entities\Entities.h"
#include "segautils\Rect.h"
#include "segautils\Defs.h"

typedef struct EntitySystem_t EntitySystem;
typedef struct SelectionManager_t SelectionManager;
typedef struct WorldView_t WorldView;

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

SelectionManager *createSelectionManager(WorldView *view);
void selectionManagerSelectEx(SelectionManager *self, SelectCriteria *filters, size_t filterCount);
vec(EntityPtr) *selectionManagerGetSelected(SelectionManager *self);



//links an arbitrary entity as a "Selection transient" to  a parent
//this gives the selection manager governance over when the entity is visible and
//makes the transient be destroyed if its parent is.
void entityLinkSelectionTransient(Entity *parent, Entity *transient);

#define selectionManagerSelect(__selManager, ...) {\
      SelectCriteria __filterList[] = { __VA_ARGS__ }; \
      selectionManagerSelectEx(__selManager, __filterList, sizeof(__filterList)/sizeof(__filterList[0])); \
   }

#define selectionManagerSelectAll(__selManager) {\
      selectionManagerSelectEx(__selManager, NULL, 0); \
   }

vec(EntityPtr) *selectionManagerGetEntitiesEX(SelectionManager *self, SelectCriteria *filters, size_t filterCount);
#define selectionManagerGetEntities(__selManager, out, ...) {\
      SelectCriteria __filterList[] = { __VA_ARGS__ }; \
      out = selectionManagerGetEntitiesEX(__selManager, __filterList, sizeof(__filterList)/sizeof(__filterList[0])); \
   }