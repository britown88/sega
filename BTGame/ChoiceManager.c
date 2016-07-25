#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"

struct ChoiceManager_t {
   Manager m;
   WorldView *view;
};

ImplManagerVTable(ChoiceManager)

ChoiceManager *createChoiceManager(WorldView *view) {
   ChoiceManager *out = checkedCalloc(1, sizeof(ChoiceManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(ChoiceManager);
   return out;
}

void _destroy(ChoiceManager *self) {
   checkedFree(self);
}
void _onDestroy(ChoiceManager *self, Entity *e) {}
void _onUpdate(ChoiceManager *self, Entity *e) {}