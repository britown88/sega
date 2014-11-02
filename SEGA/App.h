#pragma once

#include "Vector.h"
#include "Strings.h"

typedef struct GLFWmonitor GLFWmonitor;
typedef struct Renderer_t Renderer;

//app subclassing
typedef struct VirtualApp_t VirtualApp;

typedef struct {
   double frameRate;
   Int2 defaultWindowSize;
   StringView windowTitle;
   GLFWmonitor *monitor;
} AppData;

typedef struct {
   void (*destroy)(VirtualApp *);

   AppData (*getData)(VirtualApp *);

   void (*onStart)(VirtualApp *);
   void (*onStep)(VirtualApp *);
   void (*onRender)(VirtualApp *, Renderer *);

} VirtualAppVTable;

struct VirtualApp_t {VirtualAppVTable *vTable;};

static AppData virtualAppGetData(VirtualApp *self){return self->vTable->getData(self);}
static void virtualAppDestroy(VirtualApp *self){self->vTable->destroy(self);}
static void virtualAppOnStart(VirtualApp *self){self->vTable->onStart(self);}
static void virtualAppOnStep(VirtualApp *self){self->vTable->onStep(self);}
static void virtualAppOnRender(VirtualApp *self, Renderer *r){self->vTable->onRender(self, r);}

//app and functions
typedef struct App_t App;

void runApp(VirtualApp *subclass);

Int2 appGetWindowSize();

double appGetTime(App *self);
double appGetFrameTime(App *self);
double appGetFrameRate(App *self);

int appRand(App *self, int lower, int upper);

void appSleep(int ms);

extern App *g_app;




