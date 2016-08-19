#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "MeshRendering.h"
#include "WorldView.h"
#include "GridManager.h"
#include "RichText.h"

#include <stdio.h>

struct RenderManager_t{
   Manager m;
   WorldView *view;
   FontFactory *fontFactory;
   double *fps;
   bool showFPS;

   vec(EntityPtr) *layers[LayerCount];
   LayerRenderer layerRenderers[LayerCount];
};

#define ClosureTPart CLOSURE_NAME(LayerRenderer)
#include "segautils\Closure_Impl.h"

typedef struct{
   bool hasImage;
   bool hasMesh;
   bool hasRect;
   bool hasPolygon;
   bool hasText;
   bool inView;

   ManagedImage *img;
   StringView imgID;
}TRenderComponent;

static void _updateManagedImage(RenderManager *self, TRenderComponent *trc, ImageComponent *ic){
   if (trc->img && trc->imgID != ic->imgID){
      //image has changed
      managedImageDestroy(trc->img);
   }

   trc->imgID = ic->imgID;
   trc->img = imageLibraryGetImage(self->view->imageLibrary, trc->imgID);
}

static bool _buildTRenderComponent(RenderManager *self, TRenderComponent *trc, Entity *e){
   bool success = false;
   ImageComponent *ic = entityGet(ImageComponent)(e);

   trc->hasImage = ic != NULL;
   trc->inView = entityGet(InViewComponent)(e) != NULL;

   if (ic){
      _updateManagedImage(self, trc, ic);
      success = true;
   }

   trc->hasRect = entityGet(RectangleComponent)(e) != NULL;
   if (trc->hasRect){ success = true; }

   trc->hasPolygon = entityGet(PolygonComponent)(e) != NULL;
   if (trc->hasPolygon){ success = true; }

   trc->hasText = entityGet(TextComponent)(e) != NULL;
   if (trc->hasText){ success = true; }

   trc->hasMesh = entityGet(MeshComponent)(e) != NULL;
   if (trc->hasMesh){ success = true; }

   return success;
}

static void TRenderComponentDestroy(TRenderComponent *self){
   if (self->img){
      managedImageDestroy(self->img);
   }
}

#define COMP_DESTROY_FUNC TRenderComponentDestroy
#define TComponentT TRenderComponent
#include "Entities\ComponentDeclTransient.h"

void _initLayers(RenderManager *self){
   vec(EntityPtr) **first = self->layers;
   vec(EntityPtr) **last = first + LayerCount;

   while (first != last){ (*first++) = vecCreate(EntityPtr)(NULL); }
}

void _destroyLayers(RenderManager *self){
   vec(EntityPtr) **first = self->layers;
   vec(EntityPtr) **last = first + LayerCount;

   while (first != last){ vecDestroy(EntityPtr)(*first++); }
}

static void _imageComponentUpdate(RenderManager *self, Entity *e, ImageComponent *oldIC){
   ImageComponent *ic = entityGet(ImageComponent)(e);
   TRenderComponent *trc = entityGet(TRenderComponent)(e);
   if (trc){
      _updateManagedImage(self, trc, ic);
   }
}

static void _registerUpdateDelegate(RenderManager *self, EntitySystem *system){
   ComponentUpdate update;

   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_imageComponentUpdate, NULL);
   compRegisterUpdateDelegate(ImageComponent)(system, update);
}

ImplManagerVTable(RenderManager)

RenderManager *createRenderManager(WorldView *view, double *fps){
   RenderManager *out = checkedCalloc(1, sizeof(RenderManager));   
   out->view = view;
   out->m.vTable = CreateManagerVTable(RenderManager);   
   out->fps = fps;
   _initLayers(out);

   _registerUpdateDelegate(out, view->entitySystem);

#ifdef _DEBUG
   out->showFPS = true;
#else
   out->showFPS = false;
#endif

   return out;
}

void _destroy(RenderManager *self){
   fontFactoryDestroy(self->fontFactory);
   _destroyLayers(self);
   checkedFree(self);
}

void _onDestroy(RenderManager *self, Entity *e){}

void _onUpdate(RenderManager *self, Entity *e){
   TRenderComponent *trc = entityGet(TRenderComponent)(e);  
   TRenderComponent newtrc = { 0 };

   if (!trc){//no trc
      if (_buildTRenderComponent(self, &newtrc, e)){//we were able to generate one
         entityAdd(TRenderComponent)(e, &newtrc);//add it
      }
   }
   else{//trc present
      if (!_buildTRenderComponent(self, trc, e)){//on refresh, its no longer rendered
         entityRemove(TRenderComponent)(e);//remove it
      }
   }
}

void _clearLayers(RenderManager *self){
   vec(EntityPtr) **first = self->layers;
   vec(EntityPtr) **last = first + LayerCount;

   while (first != last){ vecClear(EntityPtr)(*first++); }
}

void _addToLayers(RenderManager *self, Entity* e){
   Layer layer = 0;
   LayerComponent *lc = entityGet(LayerComponent)(e);

   if (lc){
      layer = lc->layer;
   }

   if (layer < LayerCount){
      vecPushBack(EntityPtr)(self->layers[layer], &e);      
   }
}

void _renderMeshEntity(Entity *e, Frame *frame, MeshComponent *mc, short x, short y, Image *img){
   Transform t = {
      .size = { mc->size, mc->size, mc->size },
      .offset = { x, y, 0 },
      .rotation = quaternionFromAxisAngle(mc->rotNormal, mc->angle)
   };
   renderMesh(mc->vbo, mc->ibo, img, t, frame);
}

void _renderPolygon(Frame *frame, FrameRegion *vp, vec(Int2) *pList, byte color, bool open){
   if (!vecIsEmpty(Int2)(pList)){
      Int2 *begin = vecBegin(Int2)(pList);
      Int2 *end = vecEnd(Int2)(pList);
      while (begin != end - 1){
         Int2 p0 = *begin++;
         Int2 p1 = *begin;

         frameRenderLine(frame, vp, p0.x, p0.y, p1.x, p1.y, color);
      }

      if(!open && vecSize(Int2)(pList) >= 2) {
         Int2 p0 = *vecBack(Int2)(pList);
         Int2 p1 = *vecBegin(Int2)(pList);

         frameRenderLine(frame, vp, p0.x, p0.y, p1.x, p1.y, color);
      }
   }   
}

void renderManagerRenderSpan(RenderManager *self, Frame *frame, byte *x, byte *y, Span *span) {
   Font *font;
   byte bg = 0, fg = 15;//default colors

   if (span->style.flags&Style_Color) {
      bg = span->style.bg;
      fg = span->style.fg;
   }

   if (span->style.flags&Style_Invert) {
      bg |= (fg << 4);
      fg = bg & 15;
      bg >>= 4;
   }

   font = fontFactoryGetFont(self->fontFactory, bg, fg);

   frameRenderText(frame, c_str(span->string), *x, *y, font);
   *x += (byte)stringLen(span->string);
}

void _renderEntityText(RenderManager *self, Frame *frame, Entity *e) {
   //Font *font = fontFactoryGetFont(self->fontFactory, tc->bg, tc->fg);
   

   TextComponent *tc = entityGet(TextComponent)(e);
   if (tc->lines) {
      //default font
      Font *defaultFont = fontFactoryGetFont(self->fontFactory, 0, 15);

      vecForEach(TextLine, line, tc->lines, {
         byte x = line->x;
         byte y = line->y;
         vecForEach(Span, span, line->line,{
            renderManagerRenderSpan(self, frame, &x, &y, span);
         });
      });
   }

}


void _renderEntity(RenderManager *self, Entity *e, Frame *frame){
   TRenderComponent *trc = entityGet(TRenderComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   VisibilityComponent *vc = entityGet(VisibilityComponent)(e);
   int x = 0, y = 0;
   FrameRegion *vp = FrameRegionFULL;

   if (!trc || vc && !vc->shown){
      return;
   }

   if (pc){
      x = pc->x;
      y = pc->y;
   }

   if (trc->inView) {
      Viewport *view = self->view->viewport;

      x -= view->worldPos.x;
      y -= view->worldPos.y;

      vp = &view->region;
   }

   //render rect
   if (trc->hasRect){
      RectangleComponent *rc = entityGet(RectangleComponent)(e);
      SizeComponent *sc = entityGet(SizeComponent)(e);
      if (rc && sc){
         frameRenderRect(frame, vp, x, y, x + sc->x, y + sc->y, rc->color);
      }
   }

   //render image (or mesh)
   if (trc->hasImage){

      if (!trc->img) {
         _updateManagedImage(self, trc, entityGet(ImageComponent)(e));
      }

      if (trc->img) {
         Image *img = managedImageGetImage(trc->img);

         if (trc->hasMesh) {
            MeshComponent *mc = entityGet(MeshComponent)(e);
            if (mc) { _renderMeshEntity(e, frame, mc, x, y, img); }
         }
         else {
            ImageComponent *ic = entityGet(ImageComponent)(e);
            if (ic->partial) {
               frameRenderImagePartial(frame, vp, x, y, img, ic->x, ic->y, ic->width, ic->height);
            }
            else {
               frameRenderImage(frame, vp, x, y, img);
            }

         }
      }
   }

   //polygons
   if (trc->hasPolygon){
      PolygonComponent *pc = entityGet(PolygonComponent)(e);
      _renderPolygon(frame, vp, pc->pList, pc->color, pc->open);
   }

   //text
   if (trc->hasText){
      _renderEntityText(self, frame, e);         
      
   }
}

void _renderLayer(RenderManager *self, vec(EntityPtr) *layer, Frame *frame){
   vecForEach(EntityPtr, e, layer, {
      _renderEntity(self, *e, frame);
   });
}

void _renderLayers(RenderManager *self, Frame *frame){
   vec(EntityPtr) **first = self->layers;
   vec(EntityPtr) **last = first + LayerCount;

   int i = 0;
   while (first != last){
      if (!closureIsNull(LayerRenderer)(&self->layerRenderers[i])) {
         closureCall(&self->layerRenderers[i], frame);
      }
      ++i;

      _renderLayer(self, *first++, frame);  
   }
}


void _renderFramerate(Frame *frame, Font *font, double d){
   static char buffer[256] = { 0 };
   short y = 2;
   short x;
   sprintf(buffer, "FPS: %.2f", d);

   x = EGA_TEXT_RES_WIDTH - (byte)strlen(buffer) - 2;
   frameRenderText(frame, buffer, x, y, font);
}

void renderManagerToggleFPS(RenderManager *self) {
   self->showFPS = !self->showFPS;
}

void renderManagerInitialize(RenderManager *self) {
   ManagedImage *fontImg = imageLibraryGetImage(self->view->imageLibrary, stringIntern(IMG_FONT));

   if (fontImg) {
      self->fontFactory = fontFactoryCreate(managedImageGetImage(fontImg));
      managedImageDestroy(fontImg);
   }
}

#include <math.h>
#define SAMPLES 100000
#define PI  3.14159265358979323846

static int it = 0;

void renderWave(Frame *f, double freq, int color) {
   int i = 0;
   double period = 1.0 / freq;
   double sinScale = 2.0 * (PI / period);

   double period2 = 1.0 /(freq*2);
   double sinScale2 = 2.0 * (PI / period2);

   for (i = 0; i < EGA_RES_WIDTH<<4; ++i) {
      double t = (it + i) * (1.0 / SAMPLES);
      double v = sin(sinScale * t);
      double cv = cos(sinScale * t);
      double v2 = sin(sinScale2 * t);

      v = (v + v2 + 1.0) * 0.5;
      v = MAX(0.0, MIN(1.0, v));

      cv = (cv + 1.0) * 0.5;
      cv = MAX(0.0, MIN(1.0, cv));

      frameRenderPoint(f, FrameRegionFULL, i>>2, (EGA_RES_HEIGHT * v), color);
      //frameRenderPoint(f, FrameRegionFULL, i >> 2, (EGA_RES_HEIGHT * cv), color+1);


   }
   ++it;
}

void renderManagerRender(RenderManager *self, Frame *frame){
   frameClear(frame, FrameRegionFULL, 0);

   _clearLayers(self);

   //Instead of iterating over all trendercomponetns we cant assume that an entity 
   //having one means it's drawn in this frame
   //instead we should take a more manual approach   
   COMPONENT_QUERY(self->view->entitySystem, RenderedUIComponent, trc, {
      Entity *e = componentGetParent(trc, self->view->entitySystem);
      _addToLayers(self, e);
   });

   vecForEach(EntityPtr, e, gridManagerQueryEntities(self->view->managers->gridManager), {
      _addToLayers(self, *e);
   });

   _renderLayers(self, frame);

  if (self->showFPS) {
      _renderFramerate(frame, fontFactoryGetFont(self->fontFactory, 0, 15), *self->fps);
   }




   {
      renderWave(frame, 100.00, 4);
      //renderWave(frame, 440.00, 4);
      //renderWave(frame, 494.00, 5);
      //renderWave(frame, 523.25, 6);
      //renderWave(frame, 698.46, 7);
   }
}

void renderManagerAddLayerRenderer(RenderManager *self, Layer l, LayerRenderer renderer) {
   self->layerRenderers[l] = renderer;
}
void renderManagerRemoveLayerRenderer(RenderManager *self, Layer l) {
   self->layerRenderers[l] = (LayerRenderer) { 0 };
}