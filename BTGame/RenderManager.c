#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "MeshRendering.h"

#include <stdio.h>

typedef struct{
   ManagedImage *img;
   StringView filename;
}TRenderComponent;

void TRenderComponentDestroy(TRenderComponent *self){
   if (self->img){
      managedImageDestroy(self->img);
   }
}

#define COMP_DESTROY_FUNC TRenderComponentDestroy
#define TComponentT TRenderComponent
#include "Entities\ComponentDeclTransient.h"

struct RenderManager_t{
   Manager m;
   EntitySystem *system;
   FontFactory *fontFactory;
   ImageLibrary *images;
   double *fps;

   vec(EntityPtr) *layers[LayerCount];
};

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

ImplManagerVTable(RenderManager)

RenderManager *createRenderManager(EntitySystem *system, ImageLibrary *imageManager, double *fps){
   RenderManager *out = checkedCalloc(1, sizeof(RenderManager));
   Image *fontImage = imageDeserializeOptimized("assets/img/font.ega");
   out->system = system;
   out->m.vTable = CreateManagerVTable(RenderManager);
   out->fontFactory = fontFactoryCreate(fontImage);
   out->images = imageManager;
   out->fps = fps;
   _initLayers(out);

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
   ImageComponent *ic = entityGet(ImageComponent)(e);

   if (ic){
      if (trc){
         //update existing
         if (trc->filename != ic->filename){
            //image has changed
            managedImageDestroy(trc->img);
            trc->filename = ic->filename;
            trc->img = imageLibraryGetImage(self->images, trc->filename);
         }
      }
      else {
         //new image
         TRenderComponent newtrc;
         newtrc.filename = ic->filename;
         newtrc.img = imageLibraryGetImage(self->images, newtrc.filename);
         entityAdd(TRenderComponent)(e, &newtrc);
      }
   }
   else{
      if (trc){
         //no longer rendered
         entityRemove(TRenderComponent)(e);
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
      .size = (Int3){ mc->size, mc->size, mc->size },
      .offset = (Int3){ x, y, 0 },
      .rotation = quaternionFromAxisAngle(mc->rotNormal, mc->angle)
   };
   renderMesh(mc->vbo, mc->ibo, img, t, frame);
}

void _renderPolygon(Frame *frame, vec(Int2) *pList, byte color){
   if (!vecIsEmpty(Int2)(pList)){
      Int2 *begin = vecBegin(Int2)(pList);
      Int2 *end = vecEnd(Int2)(pList);
      while (begin != end - 1){
         Int2 p0 = *begin++;
         Int2 p1 = *begin;

         frameRenderLine(frame, p0.x, p0.y, p1.x, p1.y, color);
      }

      {
         Int2 p0 = *vecBack(Int2)(pList);
         Int2 p1 = *vecBegin(Int2)(pList);

         frameRenderLine(frame, p0.x, p0.y, p1.x, p1.y, color);
      }
   }   
}

void _renderEntity(Entity *e, Frame *frame){
   PositionComponent *pc = entityGet(PositionComponent)(e);
   MeshComponent *mc = entityGet(MeshComponent)(e);
   PolygonComponent *polyc = entityGet(PolygonComponent)(e);
   TRenderComponent *trc = entityGet(TRenderComponent)(e);
   int x = 0, y = 0;
   Image *img = managedImageGetImage(trc->img);

   if (img){
      if (pc){
         x = pc->x;
         y = pc->y;
      }

      if (mc){
         _renderMeshEntity(e, frame, mc, x, y, img);
      }
      else{
         frameRenderImage(frame, x, y, img);
      }
   }

   if (polyc){
      _renderPolygon(frame, polyc->pList, polyc->color);
   }
}

void _renderLayer(RenderManager *self, vec(EntityPtr) *layer, Frame *frame){
   vecForEach(EntityPtr, e, layer, {
      _renderEntity(*e, frame);
   });
}

void _renderLayers(RenderManager *self, Frame *frame){
   vec(EntityPtr) **first = self->layers;
   vec(EntityPtr) **last = first + LayerCount;

   while (first != last){  _renderLayer(self, *first++, frame);  }
}

void _renderFramerate(Frame *frame, Font *font, double d){
   static char buffer[256] = { 0 };
   short y = 0;
   short x;
   sprintf(buffer, "FPS: %.2f", d);

   x = EGA_TEXT_RES_WIDTH - strlen(buffer);
   frameRenderText(frame, buffer, x, y, font);
}

void renderManagerRender(RenderManager *self, Frame *frame){
   frameClear(frame, 0);

   _clearLayers(self);
   
   COMPONENT_QUERY(self->system, TRenderComponent, trc, {
      Entity *e = componentGetParent(trc, self->system);
      _addToLayers(self, e);
   });

   _renderLayers(self, frame);

   _renderFramerate(frame, fontFactoryGetFont(self->fontFactory, 5, 0),  *self->fps);
}