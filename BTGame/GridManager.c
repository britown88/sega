#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"

struct GridManager_t {
   Manager m;
   WorldView *view;

};

ImplManagerVTable(GridManager)

GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);


   return out;
}

void _destroy(GridManager *self) {
   checkedFree(self);
}
void _onDestroy(GridManager *self, Entity *e) {}
void _onUpdate(GridManager *self, Entity *e) {}

void gridManagerUpdate(GridManager *self) {

}