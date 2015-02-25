#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "CoreComponents.h"
#include "Managers.h"

#include <malloc.h>
#include <stddef.h> //for NULL xD
#include <string.h>
#include <stdlib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define FULLSCREEN 0

typedef struct {
   VirtualApp vApp;
   AppData data;

} BTGame;

#pragma region App_Things

static AppData _getData(BTGame *);
static void _destroy(BTGame *);
static void _onStart(BTGame *);
static void _onStep(BTGame *);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if(!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = (AppData (*)(VirtualApp *))&_getData;
      vtable->destroy = (void (*)(VirtualApp *))&_destroy;
      vtable->onStart = (void (*)(VirtualApp *))&_onStart;
      vtable->onStep = (void (*)(VirtualApp *))&_onStep;
   }

   return vtable;
}
AppData createData() {
   AppData data;

   data.defaultWindowSize = int2Create(WINDOW_WIDTH, WINDOW_HEIGHT);
   data.frameRate = 60.0;
   data.fullScreen = FULLSCREEN;
   data.windowTitle = stringIntern("sEGA: An elegant weapon for a more civilized age.");

   return data;
}
AppData _getData(BTGame *self) {
   return self->data;
}

#pragma endregion

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData(); 

   //Other constructor shit goes here


   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   checkedFree(self);
}

void _entitiesStress(){
   BTManagers managers;
   EntitySystem *es = entitySystemCreate();

   Entity *entities[100] = { 0 };
   int iterations = sizeof(entities) / sizeof(Entity*);

   int i;

   managers.renderManager = createRenderManager();
   entitySystemRegisterManager(es, managers.renderManager);

   for (i = 0; i < iterations; ++i){
      entities[i] = entityCreate(es);
      PositionComponent pc = { i * 2, i * 3 };
      entityAdd(PositionComponent)(entities[i], &pc);
      entityUpdate(entities[i]);
   }

   for (i = 0; i < iterations / 2; ++i){
      entityRemove(PositionComponent)(entities[i * 2]);
      entityUpdate(entities[i * 2]);
   }

   for (i = 0; i < iterations; ++i){
      PositionComponent *pc = entityGet(PositionComponent)(entities[i]);
      if (pc){
         int x = pc->x;
         int y = pc->y;

         Entity *e = componentGetParent(pc, es);
         entityDestroy(e);

      }
      else{
         PositionComponent pc = { -1, -1 };
         entityAdd(PositionComponent)(entities[i], &pc);
         entityUpdate(entities[i]);

      }
   }

   for (i = 0; i < iterations; ++i){
      PositionComponent *pc = entityGet(PositionComponent)(entities[i]);
      if (pc){
         entityRemove(PositionComponent)(entities[i]);
         entityUpdate(entities[i]);
      }
   }


   entitySystemDestroy(es);
   managerDestroy(managers.renderManager);
}

void _onStart(BTGame *self){ 
   Palette defPal = paletteDeserialize("assets/img/default.pal");
   Image *testImg = imageDeserialize("assets/img/test.ega");


   _entitiesStress();

   memcpy(self->vApp.currentPalette.colors, defPal.colors, EGA_PALETTE_COLORS);
   frameRenderImage(self->vApp.currentFrame, 0, 0, testImg);

   imageDestroy(testImg);
}

void _onStep(BTGame *self){

}



