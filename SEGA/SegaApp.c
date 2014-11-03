#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "SegaApp.h"
#include "App.h"
#include "Strings.h"
#include "Shader.h"
#include "VBO.h"
#include "Matrix.h"
#include "segalib\EGA.h"
#include "Renderer.h"
#include "Defs.h"
#include "FBO.h"

#include "EGADisplay.h"
#include "EGAPalette.h"
#include "segalib\CheckedMemory.h"


#include <malloc.h>
#include <stddef.h> //for NULL xD
#include <string.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define FULLSCREEN 0

typedef struct {
   VirtualApp vApp;
   AppData data;

   EGADisplay *egaDisplay;
   FBO *egaFrameBuffer;
   Rectf viewport;
} SegaApp;

static AppData _getData(VirtualApp *);
static void _destroy(VirtualApp *);
static void _onStart(VirtualApp *);
static void _onStep(VirtualApp *);
static void _onRender(VirtualApp *, Renderer *r);

static VirtualAppVTable *getVtable()
{
   static VirtualAppVTable *vtable;
   if(!vtable) {
      vtable = malloc(sizeof(VirtualAppVTable));
      vtable->getData = &_getData;
      vtable->destroy = &_destroy;
      vtable->onStart = &_onStart;
      vtable->onStep = &_onStep;
      vtable->onRender = &_onRender;
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

VirtualApp *SegaAppCreate() {
   SegaApp *r = checkedCalloc(1, sizeof(SegaApp));
   r->vApp.vTable = getVtable();
   r->data = createData();   
   return (VirtualApp*)r;
}

void _destroy(VirtualApp *self){
   SegaApp *app = (SegaApp*)self;

   if(app->egaDisplay) {
      egaDisplayDestroy(app->egaDisplay);
   }

   if(app->egaFrameBuffer) {
      fboDestroy(app->egaFrameBuffer);
   }

   checkedFree(self);
}

AppData _getData(VirtualApp *self) {
   return ((SegaApp*)self)->data;
}

Rectf _buildProportionalViewport()
{
   float rw = WINDOW_WIDTH;
   float rh = WINDOW_HEIGHT;
   float cw = EGA_RES_WIDTH * EGA_PIXEL_WIDTH;
   float ch = EGA_RES_HEIGHT * EGA_PIXEL_HEIGHT;
   float ratio = MIN(rw/cw, rh/ch);

   Rectf vp = {0.0f, 0.0f, cw * ratio, ch * ratio};
   rectfOffset(&vp, (rw - rectfWidth(&vp)) / 2.0f, (rh - rectfHeight(&vp)) / 2.0f);

   return vp;
}


void testTEXT(FontFactory *ff, Frame *frame){
   char *text2 = " Let me show you my Pokemans!!";
   int len = strlen(text2);
   int i;
   char text[30];

   memcpy(text, text2, len);

   text[0] = 1;
   text[23] = 138;
   text[29] = 1;
   for(i = 0; i < len; ++i) {
      char chars[2] = {text[i], 0};
      int index = i%15+1;
      frameRenderText(frame, chars, 28+i, 20, fontFactoryGetFont(ff, 0, index));
   }
}

void _onStart(VirtualApp *self){ 
   SegaApp *app = (SegaApp*)self;
   byte mono[2] = {0, 63};

   byte defPal[] =  {0, 1, 2, 3,  4,  5,  20, 7,  56, 57, 58, 59, 60, 61, 62, 63};
   
   FontFactory *ff;
   PNGData *textPng;
   Image *textImg;
   PNGData *png;
   Image *testImg;
   Frame *testFrame;

   textPng = pngDataCreate("assets/img/font.png");
   pngDataRender(textPng, mono, 0, 2, 2);
   textImg = pngDataCreateImage(textPng);
   pngDataDestroy(textPng);
   ff = fontFactoryCreate(textImg);
   imageDestroy(textImg);

   app->egaDisplay = egaDisplayCreate();
   app->viewport = _buildProportionalViewport();
   app->egaFrameBuffer = fboCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);

   testFrame = frameCreate();
   png = pngDataCreate("assets/img/test2.png");

   pngDataRender(png, mono, 0, 0, 16);
   egaDisplaySetPalette(app->egaDisplay, egaDisplayInternPalette(app->egaDisplay, pngDataGetPalette(png)));

   testImg = pngDataCreateImage(png);

   frameRenderImage(testFrame, 0, 0, testImg);

   testTEXT(ff, testFrame);

   egaDisplayRenderFrame(app->egaDisplay, testFrame);
   imageDestroy(testImg);
   pngDataDestroy(png);
   fontFactoryDestroy(ff);
   frameDestroy(testFrame);

}


static int test = 0;
static int testClock = 0;
void _onStep(VirtualApp *self){
   SegaApp *app = (SegaApp*)self;
   //Image *checker = buildCheckerboardImage(EGA_RES_WIDTH, EGA_RES_HEIGHT, 16, 0, 1);

   if(++testClock % 10 == 0) {
      

   }
}

static Rectf egaBounds = {0.0f, 0.0f, (float)EGA_RES_WIDTH, (float)EGA_RES_HEIGHT};

void _onRender(VirtualApp *self, Renderer *r) {
   SegaApp *app = (SegaApp*)self;

   fboBind(app->egaFrameBuffer);
   rendererPushViewport(r, egaBounds);
   egaDisplayRender(app->egaDisplay, r);
   rendererPopViewport(r); 

   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   rendererPushViewport(r, app->viewport);
   fboRender(app->egaFrameBuffer, r);
   rendererPopViewport(r);
}


