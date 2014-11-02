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

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
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
   data.monitor = NULL;
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
   //((SegaApp *)self)->data.monitor = glfwGetPrimaryMonitor();
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

static EGAPalette *palettes[64];


PNGData *png;
Image *testImg;
Frame *testFrame;

void _onStart(VirtualApp *self){ 
   SegaApp *app = (SegaApp*)self;
   EGAPalette *currentPalette = 0;
   byte mono[2] = {0, 63};

   byte defPal[] =  {0, 1, 2, 3,  4,  5,  20, 7,  56, 57, 58, 59, 60, 61, 62, 63};

   app->egaDisplay = egaDisplayCreate();
   app->viewport = _buildProportionalViewport();
   app->egaFrameBuffer = fboCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);

   testFrame = frameCreate();
   png = pngDataCreate("assets/img/test.png");   

   pngDataRender(png, defPal, 0, 0, 16);
   pngDataExportPNG(png, "assets/img/test2-ega.png");

   paletteSerialize(pngDataGetPalette(png), "test.pal");

   egaDisplaySetPalette(app->egaDisplay, egaDisplayInternPalette(app->egaDisplay, paletteDeserialize("test.pal").colors));

   testImg = pngDataCreateImage(png);
   imageSerialize(testImg, "whatever.ega");
   imageDestroy(testImg);

   testImg = imageDeserialize("whatever.ega");

   frameRenderImage(testFrame, 0, 0, testImg);
   egaDisplayRenderFrame(app->egaDisplay, testFrame);
   imageDestroy(testImg);

}


static int test = 0;
static int testClock = 0;
void _onStep(VirtualApp *self){
   //SegaApp *app = (SegaApp*)self;
   ////Image *checker = buildCheckerboardImage(EGA_RES_WIDTH, EGA_RES_HEIGHT, 16, 0, 1);

   //if(++testClock % 2 == 0) {
   //   pngDataRender(png, NULL, 0, 0, ++test%15 + 1);

   //   //pngDataRenderWithPartialPalette(png, 0, 0, 0, (++test%15 + 1));
   //   egaDisplaySetPalette(app->egaDisplay, egaDisplayInternPalette(app->egaDisplay, pngDataGetPalette(png)));

   //   testImg = pngDataCreateImage(png);
   //   //frameRenderImage(testFrame, 0, 0, checker);
   //   frameRenderImage(testFrame, 0, 0, testImg);
   //   egaDisplayRenderFrame(app->egaDisplay, testFrame);
   //   imageDestroy(testImg);
   //}
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


