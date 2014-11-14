#include "GL/glew.h"
#include <GLFW/glfw3.h>

#include "GLSLRenderer.h"
#include "segashared\CheckedMemory.h"

#include "Renderer.h"
#include "EGADisplay.h"
#include "EGAPalette.h"
#include "FBO.h"
#include "PaletteTable.h"

typedef struct {
   IRenderer ir;

   Renderer *renderer;
   EGADisplay *egaDisplay;
   FBO *egaFrameBuffer;
   PaletteTable *pTable;

} GLSLRenderer;

static void _Init(GLSLRenderer*);
static void _RenderFrame(GLSLRenderer*, Frame *, byte *, Rectf *);
static void _Destroy(GLSLRenderer*);

static IRendererVTable *_getTable() {
   static IRendererVTable *r = 0;
   if (!r){
      r = calloc(1, sizeof(IRendererVTable));
      r->init = (void(*)(IRenderer*))&_Init;
      r->renderFrame = (void(*)(IRenderer*, Frame*, byte*, Rectf *))&_RenderFrame;
      r->destroy = (void(*)(IRenderer*))&_Destroy;
   }

   return r;
}

IRenderer *createGLSLRenderer(){
   GLSLRenderer *r = checkedCalloc(1, sizeof(GLSLRenderer));
   r->ir.vTable = _getTable();

   return (IRenderer *)r;
}

void _Init(GLSLRenderer *self) {
   self->renderer = rendererCreate();
   self->egaDisplay = egaDisplayCreate();
   self->egaFrameBuffer = fboCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);
   self->pTable = paletteTableCreate();
}

static Rectf egaBounds = { 0.0f, 0.0f, (float)EGA_RES_WIDTH, (float)EGA_RES_HEIGHT };

void _RenderFrame(GLSLRenderer *self, Frame *frame, byte *palette, Rectf *viewport) {
   EGAPalette *ep = paletteTableGetPalette(self->pTable, palette);

   egaDisplaySetPalette(self->egaDisplay, ep);
   egaDisplayRenderFrame(self->egaDisplay, frame);

   glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   //render to ogl fbo
   fboBind(self->egaFrameBuffer);
   rendererPushViewport(self->renderer, egaBounds);
   egaDisplayRender(self->egaDisplay, self->renderer);
   rendererPopViewport(self->renderer);

   //render to screen
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   rendererPushViewport(self->renderer, *viewport);
   fboRender(self->egaFrameBuffer, self->renderer);
   rendererPopViewport(self->renderer);

}
void _Destroy(GLSLRenderer *self){
   paletteTableDestroy(self->pTable);
   egaDisplayDestroy(self->egaDisplay);
   fboDestroy(self->egaFrameBuffer);
   rendererDestroy(self->renderer);

   checkedFree(self);
}