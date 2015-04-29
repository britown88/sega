#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segashared\Strings.h"
#include "CoreComponents.h"
#include "Managers.h"
#include "ImageLibrary.h"

#include <malloc.h>
#include <stddef.h> //for NULL xD
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "GridManager.h"
#include "SEGA\Input.h"
#include "MeshRendering.h"
#include "WorldView.h"


#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 720
#define FULLSCREEN 0
#define FRAME_RATE 60.0

typedef struct {
   VirtualApp vApp;
   AppData data;
   BTManagers managers;
   EntitySystem *entitySystem;
   ImageLibrary *imageLibrary;

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

   data.defaultWindowSize = (Int2){ WINDOW_WIDTH, WINDOW_HEIGHT };
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




#define RegisterManager(member, funcCall) \
   member = funcCall; \
   entitySystemRegisterManager(self->entitySystem, (Manager*)member);

void _initEntitySystem(BTGame *self){
   self->entitySystem = entitySystemCreate();

   RegisterManager(self->managers.renderManager, createRenderManager(self->entitySystem, self->imageLibrary, &self->data.fps));
   RegisterManager(self->managers.cursorManager, createCursorManager(self->entitySystem));
   RegisterManager(self->managers.gridManager, createGridManager(self->entitySystem));
   RegisterManager(self->managers.interpolationManager, createInterpolationManager(self->entitySystem));
   RegisterManager(self->managers.diceManager, createDiceManager(self->entitySystem));
}

void _destroyEntitySystem(BTGame *self){
   entitySystemDestroy(self->entitySystem);

   managerDestroy((Manager*)self->managers.renderManager);
   managerDestroy((Manager*)self->managers.cursorManager);
   managerDestroy((Manager*)self->managers.gridManager);
   managerDestroy((Manager*)self->managers.interpolationManager);
   managerDestroy((Manager*)self->managers.diceManager);
}

VirtualApp *btCreate() {
   BTGame *r = checkedCalloc(1, sizeof(BTGame));
   r->vApp.vTable = getVtable();
   r->data = createData(); 

   //Other constructor shit goes here   
   r->imageLibrary = imageLibraryCreate();
  

   _initEntitySystem(r);

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   _destroyEntitySystem(self);

   imageLibraryDestroy(self->imageLibrary);
   checkedFree(self);
}

#include "segautils\FSM.h"

typedef struct {
   EMPTY_STRUCT;
}TestMessage;

CreateRTTI(TestMessage);

static void MenuStateUpdate(ClosureData data, TestMessage *msg){

}

static void TestState(ClosureData data, Type *t, Message m){
   if (t == GetRTTI(TestMessage)){
      MenuStateUpdate(data, m);
   }
}

void createTestFSM(){
   FSM *fsm = fsmCreate();
   StateClosure testState;

   closureInit(StateClosure)(&testState, NULL, (StateClosureFunc)&TestState, NULL);
   fsmPush(fsm, testState);

   fsmSend(fsm, TestMessage, 0);

   fsmDestroy(fsm);

}

void _onStart(BTGame *self){ 

   int i; 
   int foo = 0;

   appLoadPalette(appGet(), "assets/img/boardui.pal");

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

      Entity *e = entityCreate(self->entitySystem);

      COMPONENT_ADD(e, PositionComponent, 0, 0);
      COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/actor.ega"));

      COMPONENT_ADD(e, LayerComponent, LayerTokens);
      COMPONENT_ADD(e, GridComponent, 0, 0);
      COMPONENT_ADD(e, WanderComponent, 1);

      entityUpdate(e);

   }

   createTestFSM(); 
}

static bool paused = false;

static void _testKeyboard(BTGame *self){
   
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
      switch (e.key){
      case (SegaKey_Escape):
         if (e.action == SegaKey_Released){
            appQuit(appGet());
         }
         break;
      case (SegaKey_Space):
         if (e.action == SegaKey_Released){
            if (paused){
               interpolationManagerResume(self->managers.interpolationManager);
               paused = false;
            }
            else{
               interpolationManagerPause(self->managers.interpolationManager);
               paused = true;
            }
         }
         break;
      }

   }
}



static void _testMouse(){
   Mouse *k = appGetMouse(appGet());
   MouseEvent e = { 0 };
   while (mousePopEvent(k, &e)){
      if (e.action == SegaMouse_Scrolled){
         //size += e.pos.y;
      }
   }
}


void _onStep(BTGame *self){
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);
   cursorManagerUpdate(self->managers.cursorManager, mousePos.x, mousePos.y);

   interpolationManagerUpdate(self->managers.interpolationManager);
   if (!paused)
      derjpkstras(self->entitySystem, self->managers.gridManager);

   diceManagerUpdate(self->managers.diceManager);

   _testKeyboard(self);
   _testMouse();

   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);
   


   
}



