#pragma once

#include "segautils\Vector.h"
#include "segashared\Strings.h"
#include "segautils\DLLBullshit.h"
#include "segalib\EGA.h"

#include "IRenderer.h"
#include "IDeviceContext.h"

//device context flags
#define DC_FLAG_FULLSCREEN (1 << 0)

typedef struct Renderer_t Renderer;

//app subclassing
typedef struct VirtualApp_t VirtualApp;

typedef struct {
   double fps;
   double frameRate;
   Int2 defaultWindowSize;
   StringView windowTitle;
   int dcFlags; 
} AppData;

typedef struct {
   void (*destroy)(VirtualApp *);

   AppData *(*getData)(VirtualApp *);

   void (*onStart)(VirtualApp *);
   void (*onStep)(VirtualApp *);

} VirtualAppVTable;

struct VirtualApp_t {
   VirtualAppVTable *vTable;

   Frame *currentFrame;
   Palette currentPalette;
};

static AppData *virtualAppGetData(VirtualApp *self){return self->vTable->getData(self);}
static void virtualAppDestroy(VirtualApp *self){self->vTable->destroy(self);}
static void virtualAppOnStart(VirtualApp *self){self->vTable->onStart(self);}
static void virtualAppOnStep(VirtualApp *self){self->vTable->onStep(self);}

//app and functions
typedef struct App_t App;

DLL_PUBLIC void runApp(VirtualApp *subclass, IRenderer *renderer, IDeviceContext *context);

DLL_PUBLIC int appRand(App *self, int lower, int upper);
DLL_PUBLIC Int2 appGetPointerPos(App *self);

double appGetTime(App *self);
double appGetFrameTime(App *self);
double appGetFrameRate(App *self);




extern App *g_App;

//returns the global app
DLL_PUBLIC App *appGet();




