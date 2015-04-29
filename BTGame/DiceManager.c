#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"

struct MyTestManager_t{
   Manager m;
   EntitySystem *system;
};

ImplManagerVTable(MyTestManager)

MyTestManager *createMyTestManager(EntitySystem *system){
   MyTestManager *out = checkedCalloc(1, sizeof(CursorManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(MyTestManager);
   return out;
}

void _destroy(MyTestManager *self){}
void _onDestroy(MyTestManager *self, Entity *e){}
void _onUpdate(MyTestManager *self, Entity *e){}