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
#include "MeshRendering.h"


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

   vec(Vertex) *vbo;
   vec(size_t) *ibo;

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
   
   r->vbo = vecCreate(Vertex)(NULL);
   r->ibo = vecCreate(size_t)(NULL);

   _initEntitySystem(r);

   return (VirtualApp*)r;
}

void _destroy(BTGame *self){
   _destroyEntitySystem(self);

   vecDestroy(Vertex)(self->vbo);
   vecDestroy(size_t)(self->ibo);

   imageManagerDestroy(self->imageManager);   
   checkedFree(self);
}

static void buildDiceBuffers(vec(Vertex) *vbo, vec(size_t) *ibo){
   int s = 32;
   //1: 0-3

   vecPushStackArray(Vertex, vbo, {
      {.coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 0, s * 0 }},
      {.coords = {  0.5f, -0.5f, -0.5f }, .texCoords = { s * 1, s * 0 }},
      {.coords = {  0.5f,  0.5f, -0.5f }, .texCoords = { s * 1, s * 1 }},
      {.coords = { -0.5f,  0.5f, -0.5f }, .texCoords = { s * 0, s * 1 }},

      //2: 4-7
      {.coords = { 0.5f, -0.5f, -0.5f }, .texCoords = { s * 1, s * 0 }},
      {.coords = { 0.5f, -0.5f,  0.5f }, .texCoords = { s * 2, s * 0 }},
      {.coords = { 0.5f,  0.5f,  0.5f }, .texCoords = { s * 2, s * 1 }},
      {.coords = { 0.5f,  0.5f, -0.5f }, .texCoords = { s * 1, s * 1 }},

      //3: 8-11
      {.coords = { -0.5f,  0.5f, -0.5f }, .texCoords = { s * 2, s * 0 }},
      {.coords = {  0.5f,  0.5f, -0.5f }, .texCoords = { s * 3, s * 0 }},
      {.coords = {  0.5f,  0.5f,  0.5f }, .texCoords = { s * 3, s * 1 }},
      {.coords = {  -0.5f, 0.5f,  0.5f }, .texCoords = { s * 2, s * 1 }},

      //4: 12-15
      {.coords = {  0.5f, -0.5f, -0.5f }, .texCoords = { s * 0, s * 1 }},
      {.coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 1, s * 1 }},
      {.coords = { -0.5f, -0.5f,  0.5f }, .texCoords = { s * 1, s * 2 }},
      {.coords = {  0.5f, -0.5f,  0.5f }, .texCoords = { s * 0, s * 2 }},

      //5: 16-19
      {.coords = { -0.5f, -0.5f,  0.5f }, .texCoords = { s * 1, s * 1 }},
      {.coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 2, s * 1 }},
      {.coords = { -0.5f,  0.5f, -0.5f }, .texCoords = { s * 2, s * 2 }},
      {.coords = { -0.5f,  0.5f,  0.5f }, .texCoords = { s * 1, s * 2 }},

      //6: 20-23
      {.coords = {  0.5f, -0.5f, 0.5f }, .texCoords = { s * 2, s * 1 }},
      {.coords = { -0.5f, -0.5f, 0.5f }, .texCoords = { s * 3, s * 1 }},
      {.coords = { -0.5f,  0.5f, 0.5f }, .texCoords = { s * 3, s * 2 }},
      {.coords = {  0.5f,  0.5f, 0.5f }, .texCoords = { s * 2, s * 2 }}
   });


   vecPushStackArray(size_t, ibo,
   { 0, 1, 2, 0, 2, 3,
     4, 5, 6, 4, 6, 7, 
     8, 9, 10,8, 10,11, 
     12,13,14,12,14,15, 
     16,17,18,16,18,19, 
     20,21,22,20,22,23 });
}

static Image *diceTest;

void _onStart(BTGame *self){ 
   Palette defPal = paletteDeserialize("assets/img/boardui.pal");

   int i; 
   int foo = 0;

   cursorManagerCreateCursor(self->managers.cursorManager);

   diceTest = imageDeserialize("assets/img/d6.ega");
   buildDiceBuffers(self->vbo, self->ibo);

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
   paletteCopy(&self->vApp.currentPalette, &defPal);
}

static void _testKeyboard(){
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
      if (e.key == SegaKey_Escape && e.action == SegaKey_Released){
         appQuit(appGet());
      }
   }
}

static int size = 32;

static void _testMouse(){
   Mouse *k = appGetMouse(appGet());
   MouseEvent e = { 0 };
   while (mousePopEvent(k, &e)){
      if (e.action == SegaMouse_Scrolled){
         size += e.pos.y;
      }
   }
}

void _onStep(BTGame *self){
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);
   cursorManagerUpdate(self->managers.cursorManager, mousePos.x, mousePos.y);


   interpolationManagerUpdate(self->managers.interpolationManager);

   derjpkstras(self->entitySystem, self->managers.gridManager);

   _testKeyboard();
   _testMouse();

   renderManagerRender(self->managers.renderManager, self->vApp.currentFrame);
     
   
   renderMesh(self->vbo, self->ibo, diceTest, (Transform){ .size = (Int3){ size, size, size }, .offset = (Int3){ mousePos.x, mousePos.y, 0 } }, self->vApp.currentFrame);


   
   
}



