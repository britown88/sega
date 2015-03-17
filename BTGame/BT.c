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
#include "SEGA\Input.h"

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

   self->managers.gridManager = createGridManager(self->entitySystem);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.gridManager);

   self->managers.interpolationManager = createInterpolationManager(self->entitySystem);
   entitySystemRegisterManager(self->entitySystem, (Manager*)self->managers.interpolationManager);

}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
   managerDestroy((Manager*)self->managers.cursorManager);
   managerDestroy((Manager*)self->managers.gridManager);
   managerDestroy((Manager*)self->managers.interpolationManager);
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
   int foo = 0;

   cursorManagerCreateCursor(self->managers.cursorManager);

   {
      Entity *e = entityCreate(self->entitySystem);
      
      COMPONENT_ADD(e, PositionComponent, 0, 0);
      COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/boardui.ega"));
     // COMPONENT_ADD(e, LayerComponent, LayerTokens);
      entityUpdate(e);
   }   

   for (i = 0; i < CELL_COUNT - 12; ++i){

      Entity *e = entityCreate(self->entitySystem);

      COMPONENT_ADD(e, PositionComponent, 0, 0);
      COMPONENT_ADD(e, ImageComponent, stringIntern(foo++ % 2 ? "assets/img/actor.ega" : "assets/img/badguy.ega"));

      COMPONENT_ADD(e, LayerComponent, LayerTokens);;
      COMPONENT_ADD(e, GridComponent, i%TABLE_WIDTH, i/TABLE_WIDTH);
      COMPONENT_ADD(e, WanderComponent, 1);

      entityUpdate(e);
      
   }

   {

      //Entity *e = entityCreate(self->entitySystem);

      //COMPONENT_ADD(e, PositionComponent, 0, 0);
      //COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/actor.ega"));

      //COMPONENT_ADD(e, LayerComponent, LayerTokens);
      //COMPONENT_ADD(e, GridComponent, 0, 0);
      //COMPONENT_ADD(e, WanderComponent, 1);

      //entityUpdate(e);

   }
   paletteCopy(&self->vApp.currentPalette, &defPal);
}

static void _testKeyboard(){
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
      if (e.key == SegaKey_A && e.action == SegaKey_Released){
         int i = 5;
         i += 5;
      }
   }
}

static void _testMouse(){
   Mouse *k = appGetMouse(appGet());
   MouseEvent e = { 0 };
   while (mousePopEvent(k, &e)){
      if (e.button == SegaMouseBtn_Left && e.action == SegaMouse_Released){
         int i = 5;
         i += 5;
      }
   }
}


void _onStep(BTGame *self){
   Mouse *mouse = appGetMouse(appGet());
   if (mouse){
      Int2 mousePos = mouseGetPosition(mouse);
      cursorManagerUpdate(self->managers.cursorManager, mousePos.x, mousePos.y);
   }

   interpolationManagerUpdate(self->managers.interpolationManager);

   derjpkstras(self->entitySystem, self->managers.gridManager);

   _testKeyboard();
   _testMouse();

   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);


   

   
}



