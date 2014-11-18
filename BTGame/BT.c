#include "BT.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"
#include "segautils\IntrusiveHeap.h"

#include <malloc.h>
#include <stddef.h> //for NULL xD
#include <string.h>
#include <stdlib.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
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

typedef struct{
   int ID;
   QueueNode node;
}Thing;

static Thing *thingMinCompare(Thing *t1, Thing *t2){
   return t1->ID < t2->ID ? t1 : t2;
}

void _onStart(BTGame *self){ 
   byte defPal[] =  {0, 1, 2, 3,  4,  5,  20, 7,  56, 57, 58, 59, 60, 61, 62, 63};
   Image *testImg;
   PNGData *png = pngDataCreate("assets/img/test.png");
   PriorityQueue *pq = priorityQueueCreate(offsetof(Thing, node), (PQCompareFunc)&thingMinCompare);
   int i;
   int iterations = 100000;//ONE MIIIIILLIIIIOOOOOOON iterations
   Thing *things = checkedCalloc(iterations, sizeof(Thing));

   for (i = 0; i < iterations; ++i){
      Thing *thing = things + i;
      thing->ID = rand();     

      priorityQueuePush(pq, thing); 
   }

   for (i = 0; i < iterations; ++i){
      Thing *thing = priorityQueuePop(pq);

      int id = thing->ID;
      
   }

   checkedFree(things);
   priorityQueueDestroy(pq);


   pngDataRender(png, paletteCreatePartial(defPal, 0, 0, 16).colors);
   memcpy(self->vApp.currentPalette.colors, pngDataGetPalette(png), EGA_PALETTE_COLORS);
   testImg = pngDataCreateImage(png);
   frameRenderImage(self->vApp.currentFrame, 0, 0, testImg);

   imageDestroy(testImg);
   pngDataDestroy(png);
}

void _onStep(BTGame *self){

}



