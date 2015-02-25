#include "Managers.h"

#include "Entities\Entities.h"

#include "CoreComponents.h"
#include "segashared\CheckedMemory.h"
#include "segalib\EGA.h"

typedef struct{
   Image *img;
   StringView filename;
}TRenderComponent;

void TRenderComponentDestroy(TRenderComponent *self){
   if (self->img){
      imageDestroy(self->img);
   }
}

#define COMP_DESTROY_FUNC TRenderComponentDestroy
#define TComponentT TRenderComponent
#include "Entities\ComponentDeclTransient.h"

struct RenderManager_t{
   Manager m;
   EntitySystem *system;
   FontFactory *fontFactory;

   
};

#pragma region vtable things
void renderManagerDestroy(RenderManager*);
void renderManagerOnDestroy(RenderManager*, Entity*);
void renderManagerOnUpdate(RenderManager*, Entity*);

ManagerVTable *_createVTable(){
   static ManagerVTable *out = NULL;

   if (!out){
      out = calloc(1, sizeof(ManagerVTable));
      out->destroy = (void(*)(Manager*))&renderManagerDestroy;
      out->onDestroy = (void(*)(Manager*, Entity*))&renderManagerOnDestroy;
      out->onUpdate = (void(*)(Manager*, Entity*))&renderManagerOnUpdate;
   }

   return out;
}

#pragma endregion

RenderManager *createRenderManager(EntitySystem *system){
   RenderManager *out = checkedCalloc(1, sizeof(RenderManager));
   Image *fontImage = imageDeserialize("assets/img/font.ega");
   out->system = system;
   out->m.vTable = _createVTable();
   out->fontFactory = fontFactoryCreate(fontImage);

   imageDestroy(fontImage);
   return out;
}

void renderManagerDestroy(RenderManager *self){
   fontFactoryDestroy(self->fontFactory);
   checkedFree(self);
}

void renderManagerOnDestroy(RenderManager *self, Entity *e){}

void renderManagerOnUpdate(RenderManager *self, Entity *e){
   TRenderComponent *trc = entityGet(TRenderComponent)(e);   
   ImageComponent *ic = entityGet(ImageComponent)(e);

   if (ic){
      if (trc){
         //update existing
         if (trc->filename != ic->filename){
            //image has changed
            imageDestroy(trc->img);
            trc->filename = ic->filename;
            trc->img = imageDeserialize(trc->filename);
         }
      }
      else {
         //new image
         TRenderComponent newtrc;
         newtrc.filename = ic->filename;
         newtrc.img = imageDeserialize(newtrc.filename);
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

void renderManagerRender(RenderManager *self, Frame *frame){
   static long frameIndex = 0;
   byte bgPaletteColor = (frameIndex / 50) % 16;
   byte txtX = (frameIndex / 5) % EGA_TEXT_RES_WIDTH;
   byte txtY = (frameIndex / 5) % EGA_TEXT_RES_HEIGHT;
   ++frameIndex;
   frameClear(frame, bgPaletteColor);

   frameRenderText(frame, "OBEY", txtX, txtY, fontFactoryGetFont(self->fontFactory, bgPaletteColor, 15));
   
   COMPONENT_QUERY(self->system, TRenderComponent, trc, {
      Entity *e = componentGetParent(trc, self->system);
      PositionComponent *pc = entityGet(PositionComponent)(e);
      int x = 0, y = 0;

      if (pc){
         x = pc->x;
         y = pc->y;
      }

      frameRenderImage(frame, x, y, trc->img);   
   });
}