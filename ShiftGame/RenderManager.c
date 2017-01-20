#include "Managers.h"
#include "ImageLibrary.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"
#include "MeshRendering.h"
#include "WorldView.h"

#include <stdio.h>

struct RenderManager_t{
   Manager m;
   WorldView *view;
   FontFactory *fontFactory;
   double *fps;

   vec(EntityPtr) *layers[LayerCount];
};

typedef struct{
   bool hasImage;
   bool hasMesh;
   bool hasRect;
   bool hasPolygon;
   bool hasText;

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
   Image *fontImage = imageDeserialize("assets/img/font.ega", EGA_IMGD_LEGACY );
   Texture *fontTex = imageCreateTexture(fontImage);
   out->view = view;
   out->m.vTable = CreateManagerVTable(RenderManager);
   out->fontFactory = fontFactoryCreate(fontTex);
   out->fps = fps;
   _initLayers(out);

   _registerUpdateDelegate(out, view->entitySystem);

   imageDestroy(fontImage);
   textureDestroy(fontTex);
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
      .size = (Int3){ mc->size, mc->size, mc->size },
      .offset = (Int3){ x, y, 0 },
      .rotation = quaternionFromAxisAngle(mc->rotNormal, mc->angle)
   };
   renderMesh(mc->vbo, mc->ibo, img, t, frame);
}

void _renderPolygon(Frame *frame, vec(Int2) *pList, byte color, bool open){
   if (!vecIsEmpty(Int2)(pList)){
      Int2 *begin = vecBegin(Int2)(pList);
      Int2 *end = vecEnd(Int2)(pList);
      while (begin != end - 1){
         Int2 p0 = *begin++;
         Int2 p1 = *begin;

         frameRenderLine(frame, FrameRegionFULL, p0.x, p0.y, p1.x, p1.y, color);
      }

      if(!open && vecSize(Int2)(pList) >= 2) {
         Int2 p0 = *vecBack(Int2)(pList);
         Int2 p1 = *vecBegin(Int2)(pList);

         frameRenderLine(frame, FrameRegionFULL, p0.x, p0.y, p1.x, p1.y, color);
      }
   }   
}

void _renderEntity(RenderManager *self, Entity *e, Frame *frame){
   TRenderComponent *trc = entityGet(TRenderComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   VisibilityComponent *vc = entityGet(VisibilityComponent)(e);
   int x = 0, y = 0;

   if (vc && !vc->shown){
      return;
   }

   if (pc){
      x = pc->x;
      y = pc->y;
   }

   //render rect
   if (trc->hasRect){
      RectangleComponent *rc = entityGet(RectangleComponent)(e);
      SizeComponent *sc = entityGet(SizeComponent)(e);
      if (rc && sc){
         frameRenderRect(frame, FrameRegionFULL, x, y, x + sc->x, y + sc->y, rc->color);
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
            frameRenderImagePartial(frame, FrameRegionFULL, x, y, img, ic->x, ic->y, ic->width, ic->height);
         }
         else{
            frameRenderImage(frame, FrameRegionFULL, x, y, img);
         }
         
      }
   }

   //polygons
   if (trc->hasPolygon){
      PolygonComponent *pc = entityGet(PolygonComponent)(e);
      _renderPolygon(frame, pc->pList, pc->color, pc->open);
   }

   //text
   if (trc->hasText){
      TextComponent *tc = entityGet(TextComponent)(e);
      frameRenderText(frame, c_str(tc->text), tc->x, tc->y,
         fontFactoryGetFont(self->fontFactory, tc->bg, tc->fg));
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

   while (first != last){  _renderLayer(self, *first++, frame);  }
}


void _renderFramerate(Frame *frame, Font *font, double d){
   static char buffer[256] = { 0 };
   short y = 0;
   short x;
   sprintf(buffer, "FPS: %.2f", d);

   x = EGA_TEXT_RES_WIDTH - (byte)strlen(buffer);
   frameRenderText(frame, buffer, x, y, font);
}

void renderManagerRender(RenderManager *self, Frame *frame){
   frameClear(frame, FrameRegionFULL, 0);

   _clearLayers(self);
   
   COMPONENT_QUERY(self->view->entitySystem, TRenderComponent, trc, {
      Entity *e = componentGetParent(trc, self->view->entitySystem);
      _addToLayers(self, e);
   });

   _renderLayers(self, frame);

   _renderFramerate(frame, fontFactoryGetFont(self->fontFactory, 7, 0), *self->fps);
}