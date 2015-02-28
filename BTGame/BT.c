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

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 720
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

}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
   managerDestroy((Manager*)self->managers.cursorManager);
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

   short gridX = 224;
   short gridY = 38;
   short sqx = 12, sqy = 8;
   short gridSize = 32;

   for (i = 0; i < sqx*sqy; ++i){

      Entity *e;

      appGet();
      
      if (appRand(appGet(), 0, 5) == 0){
         e = entityCreate(self->entitySystem);

         //ADD_NEW_COMPONENT(e, PositionComponent, rand() % EGA_RES_WIDTH, rand() % EGA_RES_HEIGHT);

         ADD_NEW_COMPONENT(e, PositionComponent, gridX + (i%sqx)*gridSize, gridY + (i / sqx)*gridSize);
         ADD_NEW_COMPONENT(e, ImageComponent, stringIntern("assets/img/actor.ega"));
         //ADD_NEW_COMPONENT(e, VelocityComponent, ((rand() % 3) + 1)*(rand() % 2 ? 1 : -1), ((rand() % 3) + 1)*(rand() % 2 ? 1 : -1));

         ADD_NEW_COMPONENT(e, LayerComponent, LayerTokens);;

         entityUpdate(e);
      }

      
   }
   paletteCopy(&self->vApp.currentPalette, &defPal);
}

void _onStep(BTGame *self){
   Int2 mousePos = appGetPointerPos(appGet());
   cursorManagerUpdate(self->managers.cursorManager, mousePos.x, mousePos.y);
   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);


   COMPONENT_QUERY(self->entitySystem, VelocityComponent, vc, {
      PositionComponent *pc = entityGet(PositionComponent)(componentGetParent(vc, self->entitySystem));
      if (pc){
         pc->x += vc->x;
         if (pc->x > EGA_RES_WIDTH){ pc->x = -16; }
         if (pc->x < -16){ pc->x = EGA_RES_WIDTH; }
         pc->y += vc->y;
         if (pc->y > EGA_RES_HEIGHT){ pc->y = -16; }
         if (pc->y < -16){ pc->y = EGA_RES_HEIGHT; }
      }
      //pc->y = (int)(sin(pc->x*(3.14 / 180.0)) * 175) + 175;

   });
}



