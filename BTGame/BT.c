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
#include "segalib\Rasterizer.h"
#include "segautils\BitTwiddling.h"

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
      if (e.key == SegaKey_Escape && e.action == SegaKey_Released){
         appQuit(appGet());
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

typedef struct{
   byte colors[3];
}TestTriangleData;

static void drawMuhPixels(Frame *f, TestTriangleData *data, TrianglePoint *p){
   int i = 0;

   if (p->pos.x < 0 || p->pos.x >= EGA_RES_WIDTH ||
      p->pos.y < 0 || p->pos.y >= EGA_RES_HEIGHT){
      return;
   }

   int closest = 0;
   for (i = 1; i < 3; ++i){
      if (p->b[i] > p->b[closest]){
         closest = i;
      }
   }

   for (i = 0; i < EGA_PLANES; ++i){
      byte bit = getBit(data->colors[closest], i);
      setBitInArray(f->planes[i].lines[p->pos.y].pixels, p->pos.x, bit);
   }
}

static Image *testImg = NULL;

typedef struct{
   Int2 texCoords[3];
}TexData;

static void drawMuhImage(Frame *f, TexData *data, TrianglePoint *p){
   int i = 0;
   float texX = 0.0f, texY = 0.0f;
   int x, y;
   static byte buff[MAX_IMAGE_WIDTH];

   if (p->pos.x < 0 || p->pos.x >= EGA_RES_WIDTH ||
      p->pos.y < 0 || p->pos.y >= EGA_RES_HEIGHT){
      return;
   }

   for (i = 0; i < 3; ++i){
      texX += data->texCoords[i].x * p->b[i];
      texY += data->texCoords[i].y * p->b[i];
   }

   x = abs((int)texX) % imageGetWidth(testImg);
   y = abs((int)texY) % imageGetHeight(testImg);

   imageScanLineRender(imageGetScanLine(testImg, y, 0), buff);
   if (!getBitFromArray(buff, x)){
      for (i = 0; i < EGA_PLANES; ++i){
         ImageScanLine *sl = imageGetScanLine(testImg, y, i + 1);
         imageScanLineRender(sl, buff);

         setBitInArray(f->planes[i].lines[p->pos.y].pixels, p->pos.x, getBitFromArray(buff, x));
      }
   }
}

static void _testTriangle(Frame *f, Int2 mousePos){

      PixelShader pix, tex;
      //closureInit(PixelShader)(&pix, f, (PixelShaderFunc)&drawMuhPixels, NULL);
      //drawTriangle(pix, &(TestTriangleData){5, 10, 12}, (Int2){ 100, 100 }, (Int2){ 200, 100 }, mousePos);
      //drawTriangle(pix, &(TestTriangleData){12, 15, 5}, mousePos, (Int2){ 100, 200 }, (Int2){ 100, 100 });

      if (!testImg){
         testImg = imageDeserializeOptimized("assets/img/badguy.ega");
      }

      closureInit(PixelShader)(&tex, f, (PixelShaderFunc)&drawMuhImage, NULL);
      drawTriangle(tex, &(TexData){{{0, 0}, { 31, 0 }, { 31, 31 }}}, (Int2){ 100, 100 }, (Int2){ 200, 100 }, mousePos);
      drawTriangle(tex, &(TexData){{{31, 31}, { 0, 31 }, { 0, 0 }}}, mousePos, (Int2){ 100, 200 }, (Int2){ 100, 100 });



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

   _testTriangle(self->vApp.currentFrame, mousePos);
   
}



