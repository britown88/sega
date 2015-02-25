#include "Managers.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"

struct RenderManager_t{
   Manager m;
};

void renderManagerDestroy(RenderManager*);
void renderManagerOnDestroy(RenderManager*, Entity*);
void renderManagerOnUpdate(RenderManager*, Entity*);

ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&renderManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&renderManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&renderManagerOnUpdate;
   }

   return out;
}

RenderManager *createRenderManager(){
   RenderManager *out = checkedCalloc(1, sizeof(RenderManager));

   out->m.vTable = _createVTable();

   return out;
}

void renderManagerDestroy(RenderManager *self){
   int i = 'fuck';
   i += ' you';
}
void renderManagerOnDestroy(RenderManager *self, Entity *e){
   int i = 'fuck';
   i += ' you';
}
void renderManagerOnUpdate(RenderManager *self, Entity *e){
   int i = 'fuck';
   i += ' you';
}