#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "MeshRendering.h"
#include "WorldView.h"
#include "GridManager.h"

#include <stdio.h>

struct RenderManager_t{
   Manager m;
   WorldView *view;
   FontFactory *fontFactory;
   double *fps;
   bool showFPS;

   vec(EntityPtr) *layers[LayerCount];
};

typedef struct{
   bool hasImage;
   bool hasMesh;
   bool hasRect;
   bool hasPolygon;
   bool hasText;
   bool inView;

   ManagedImage *img;
   StringView filename;
}TRenderComponent;

static void _updateManagedImage(RenderManager *self, TRenderComponent *trc, ImageComponent *ic){
   if (trc->img && trc->filename != ic->filename){
      //image has changed
      managedImageDestroy(trc->img);
      trc->filename = ic->filename;
      trc->img = imageLibraryGetImage(self->view->imageLibrary, trc->filename);
   }
   else {
      //new image
      trc->filename = ic->filename;
      trc->img = imageLibraryGetImage(self->view->imageLibrary, trc->filename);
   }
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
   Image *fontImage = imageDeserializeOptimized("assets/img/cga8bold.ega");
   out->view = view;
   out->m.vTable = CreateManagerVTable(RenderManager);
   out->fontFactory = fontFactoryCreate(fontImage);
   out->fps = fps;
   _initLayers(out);

   _registerUpdateDelegate(out, view->entitySystem);

#ifdef _DEBUG
   out->showFPS = true;
#else
   out->showFPS = false;
#endif

   imageDestroy(fontImage);
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

void _renderEntity(RenderManager *self, Entity *e, Frame *frame){
   TRenderComponent *trc = entityGet(TRenderComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   VisibilityComponent *vc = entityGet(VisibilityComponent)(e);
   int x = 0, y = 0;
   FrameRegion *vp = FrameRegionFULL;

   if (vc && !vc->shown){
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
   if (trc->hasImage && trc->img){
      Image *img = managedImageGetImage(trc->img);

      if (trc->hasMesh){
         MeshComponent *mc = entityGet(MeshComponent)(e);
         if (mc){ _renderMeshEntity(e, frame, mc, x, y, img); }         
      }
      else{
         ImageComponent *ic = entityGet(ImageComponent)(e);
         if (ic->partial){
            frameRenderImagePartial(frame, vp, x, y, img, ic->x, ic->y, ic->width, ic->height);
         }
         else{
            frameRenderImage(frame, vp, x, y, img);
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
      TextComponent *tc = entityGet(TextComponent)(e);
      if (tc->lines) {
         vecForEach(TextLine, line, tc->lines, {
            frameRenderText(frame, c_str(line->text), line->x, line->y, fontFactoryGetFont(self->fontFactory, tc->bg, tc->fg));
         });

      }
      
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
      switch (i++) {
      case LayerGrid:
         gridManagerRender(self->view->managers->gridManager, frame);
         break;
      case LayerGridLighting:
         gridManagerRenderLighting(self->view->managers->gridManager, frame);
         break;
      }

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

   frameRenderText(frame, "You are likely to be", 16, 22, fontFactoryGetFont(self->fontFactory, 0, 15));
   frameRenderText(frame, "eaten by a grue.", 16, 23, fontFactoryGetFont(self->fontFactory, 0, 15));

   //frameRenderText(frame, "         Move        Brightness", 2, 22, fontFactoryGetFont(self->fontFactory, 0, 15));
   //frameRenderText(frame, "        Radius           Place Light", 2, 23, fontFactoryGetFont(self->fontFactory, 0, 15));

   //frameRenderText(frame, "W,A,S,D:", 2, 22, fontFactoryGetFont(self->fontFactory, 0, 14));
   //frameRenderText(frame, "Scroll:", 2, 23, fontFactoryGetFont(self->fontFactory, 0, 14));
   //frameRenderText(frame, "+,-:", 18, 22, fontFactoryGetFont(self->fontFactory, 0, 14));
   //frameRenderText(frame, "L-Click:", 18, 23, fontFactoryGetFont(self->fontFactory, 0, 14));
}