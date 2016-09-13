#pragma once

#include "segautils\Vector.h"
#include "segautils/Time.h"
#include "segashared\Strings.h"
#include "segautils\DLLBullshit.h"
#include "segalib\EGA.h"

#include "segautils/StandardVectors.h"
#include "IRenderer.h"
#include "IDeviceContext.h"

#include "segautils/extern_c.h"

//device context flags
#define DC_FLAG_FULLSCREEN (1 << 0)

typedef struct Renderer_t Renderer;

//app subclassing
typedef struct VirtualApp_t VirtualApp;

typedef struct {
   double fps;
   double desiredFrameRate;
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

SEXTERN_C

DLL_PUBLIC void runApp(VirtualApp *subclass, IRenderer *renderer, IDeviceContext *context);

DLL_PUBLIC int appRand(App *self, int lower, int upper);
DLL_PUBLIC Int2 appWindowToWorld(App *self, Float2 coords);

DLL_PUBLIC Microseconds appGetTime(App *self);
Microseconds appGetFrameTime(App *self);
double appGetFrameRate(App *self);

extern App *g_App;

//returns the global app
DLL_PUBLIC App *appGet();

DLL_PUBLIC void appQuit(App *app);

typedef struct Keyboard_t Keyboard;
DLL_PUBLIC Keyboard *appGetKeyboard(App *self);

typedef struct Mouse_t Mouse;
DLL_PUBLIC Mouse *appGetMouse(App *self);

DLL_PUBLIC Palette *appGetPalette(App *self);
DLL_PUBLIC int appLoadPalette(App *self, const char *palFile);
DLL_PUBLIC void appSetPalette(App *self, Palette *p);

#define APP_FILE_ALL 1 //returns both directories and files
#define APP_FILE_DIR_ONLY 2  //returns only directories
#define APP_FILE_FILE_ONLY 3 //return only files

DLL_PUBLIC int appListFiles(const char *root, int type, vec(StringPtr) **out, const char *ext);

END_SEXTERN_C




