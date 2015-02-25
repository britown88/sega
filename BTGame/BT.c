#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"
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
   BTManagers managers;
   EntitySystem *entitySystem;

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

void _initEntitySystem(BTGame *self){
   self->entitySystem = entitySystemCreate();

   self->managers.renderManager = createRenderManager(self->entitySystem);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.renderManager);

}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData(); 

   //Other constructor shit goes here   
   _initEntitySystem(r);

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   _destroyEntitySystem(self);
   
   checkedFree(self);
}


void _onStart(BTGame *self){ 
   Palette defPal = paletteDeserialize("assets/img/default.pal");
   //Image *testImg = imageDeserialize("assets/img/test.ega");
   int i;

   for (i = 0; i < 10; ++i){
      Entity *e = entityCreate(self->entitySystem);
      PositionComponent pc = { i * 40, 100 };
      ImageComponent img = { stringIntern("assets/img/aramis.ega") };
      entityAdd(PositionComponent)(e, &pc);
      entityAdd(ImageComponent)(e, &img);
      entityUpdate(e);
   }

   memcpy(self->vApp.currentPalette.colors, defPal.colors, EGA_PALETTE_COLORS);
   //frameRenderImage(self->vApp.currentFrame, 0, 0, testImg);

   //imageDestroy(testImg);
}

void _onStep(BTGame *self){
   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);
}



