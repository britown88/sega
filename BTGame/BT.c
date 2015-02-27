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
   data.fullScreen = FULLSCREEN;
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
   Palette defPal = paletteDeserialize("assets/img/default.pal");

   int i;

   {
      Entity *e = entityCreate(self->entitySystem);

      
      ADD_NEW_COMPONENT(e, PositionComponent, 0, 0);
      ADD_NEW_COMPONENT(e, ImageComponent, stringIntern("assets/img/adam.ega"));
      ADD_NEW_COMPONENT(e, LayerComponent, LayerTokens);
      //ADD_NEW_COMPONENT(e, VelocityComponent, 0);

      entityUpdate(e);
   }

   for (i = 0; i < MAX_ENTITIES - 1; ++i){
      Entity *e = entityCreate(self->entitySystem);

      ADD_NEW_COMPONENT(e, VelocityComponent, 0);
      ADD_NEW_COMPONENT(e, PositionComponent, i * 10, 100);
      ADD_NEW_COMPONENT(e, ImageComponent, stringIntern("assets/img/aramis.ega"));
      
      ADD_NEW_COMPONENT(e, LayerComponent, LayerBackground);;
      
      entityUpdate(e);
   }


   memcpy(self->vApp.currentPalette.colors, defPal.colors, EGA_PALETTE_COLORS);
   //frameRenderImage(self->vApp.currentFrame, 0, 0, testImg);

   //imageDestroy(testImg);
}

void _onStep(BTGame *self){
   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);

   COMPONENT_QUERY(self->entitySystem, PositionComponent, pc, {
      if (entityGet(VelocityComponent)(componentGetParent(pc, self->entitySystem))){
         pc->x++;
         pc->y = (int)(sin(pc->x*(3.14 / 180.0)) * 175) + 175;
      }
   });
}



