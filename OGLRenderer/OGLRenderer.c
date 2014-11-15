#include "defined_gl.h"
#include "OGLRenderer.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"

#include "EGATexture.h"


typedef struct {
	IRenderer ir;

	EGATexture *tex;

} OGLRenderer;

static void _Init(OGLRenderer*);
static void _RenderFrame(OGLRenderer*, Frame *, byte *, Rectf *);
static void _Destroy(OGLRenderer*);

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

IRenderer *createOGLRenderer(){
	OGLRenderer *r = checkedCalloc(1, sizeof(OGLRenderer));
   r->ir.vTable = _getTable();

   return (IRenderer *)r;
}

void _Init(OGLRenderer *self) {
	self->tex = egaTextureCreate();
}

void _RenderFrame(OGLRenderer *self, Frame *frame, byte *palette, Rectf *vp) {
	glViewport((int)vp->left, (int)vp->top, (int)rectfWidth(vp), (int)rectfHeight(vp));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, EGA_RES_WIDTH, EGA_RES_HEIGHT, 0, 1.f, -1.f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	egaTextureRenderFrame(self->tex, frame, palette);
}
void _Destroy(OGLRenderer *self){
	if (self->tex){
		egaTextureDestroy(self->tex);
	}
   checkedFree(self);
}