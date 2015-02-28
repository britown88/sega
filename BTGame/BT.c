#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"
#include "CoreComponents.h"
#include "Managers.h"
#include "ImageManager.h"

#include <malloc.h>
#include <stddef.h> //for NULL xD
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "GridManager.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define FULLSCREEN 0
#define FRAME_RATE 60.0

typedef struct {
   VirtualApp vApp;
   AppData data;
   BTManagers managers;
   EntitySystem *entitySystem;
   ImageManager *imageManager;

} BTGame;

#pragma region App_Things

static AppData *_getData(BTGame *);
static void _destroy(BTGame *);
static void _onStart(BTGame *);
static void _onStep(BTGame *);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if(!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = (AppData *(*)(VirtualApp *))&_getData;
      vtable->destroy = (void (*)(VirtualApp *))&_destroy;
      vtable->onStart = (void (*)(VirtualApp *))&_onStart;
      vtable->onStep = (void (*)(VirtualApp *))&_onStep;
   }

   return vtable;
}
AppData createData() {
   AppData data = { 0 };
   
   data.defaultWindowSize = int2Create(WINDOW_WIDTH, WINDOW_HEIGHT);
   data.frameRate = FRAME_RATE;

   if (FULLSCREEN){
      data.dcFlags |= DC_FLAG_FULLSCREEN;
   }

   data.windowTitle = stringIntern("sEGA: An elegant weapon for a more civilized age.");

   return data;
}
AppData *_getData(BTGame *self) {
   return &self->data;
}

#pragma endregion

void _initEntitySystem(BTGame *self){
   self->entitySystem = entitySystemCreate();

   self->managers.renderManager = createRenderManager(self->entitySystem, self->imageManager, &self->data.fps);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.renderManager);

   self->managers.cursorManager = createCursorManager(self->entitySystem);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.cursorManager);

   self->managers.gridManager = createGridManager(self->entitySystem);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.gridManager);

}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
   managerDestroy((Manager*)self->managers.cursorManager);
   managerDestroy((Manager*)self->managers.gridManager);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData(); 

   //Other constructor shit goes here   
   r->imageManager = imageManagerCreate();

   _initEntitySystem(r);

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   _destroyEntitySystem(self);

   imageManagerDestroy(self->imageManager);   
   checkedFree(self);
}


void _onStart(BTGame *self){ 
   Palette defPal = paletteDeserialize("assets/img/boardui.pal");

   int i;   

   cursorManagerCreateCursor(self->managers.cursorManager);

   {
      Entity *e = entityCreate(self->entitySystem);
      
      ADD_NEW_COMPONENT(e, PositionComponent, 0, 0);
      ADD_NEW_COMPONENT(e, ImageComponent, stringIntern("assets/img/boardui.ega"));
     // ADD_NEW_COMPONENT(e, LayerComponent, LayerTokens);
      entityUpdate(e);
   }

   

   for (i = 0; i < CELL_COUNT; ++i){

      Entity *e;

      appGet();
      
      if (appRand(appGet(), 0, 5) == 0){
         e = entityCreate(self->entitySystem);

         ADD_NEW_COMPONENT(e, PositionComponent, 0, 0);
         ADD_NEW_COMPONENT(e, ImageComponent, stringIntern("assets/img/actor.ega"));

         ADD_NEW_COMPONENT(e, LayerComponent, LayerTokens);;
         ADD_NEW_COMPONENT(e, GridComponent, i%TABLE_WIDTH, i/TABLE_WIDTH);;

         entityUpdate(e);
      }

      
   }
   paletteCopy(&self->vApp.currentPalette, &defPal);
}

void _onStep(BTGame *self){
   
   Int2 mousePos = appGetPointerPos(appGet());
   cursorManagerUpdate(self->managers.cursorManager, mousePos.x, mousePos.y);

   gridManagerUpdate(self->managers.gridManager);
   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);
   
}



