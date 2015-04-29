#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"

struct DiceManager_t{
   Manager m;
   EntitySystem *system;
   Entity *die;

   vec(Vertex) *vbo;
   vec(size_t) *ibo;
};

static void _buildDiceBuffers(vec(Vertex) *vbo, vec(size_t) *ibo){
   int s = 32;

   vecPushStackArray(Vertex, vbo, {
      //1: 0-3
      { .coords = { -0.5f, -0.5f, 0.5f }, .texCoords = { s * 0, s * 0 } },
      { .coords = { 0.5f, -0.5f, 0.5f }, .texCoords = { s * 1, s * 0 } },
      { .coords = { 0.5f, 0.5f, 0.5f }, .texCoords = { s * 1, s * 1 } },
      { .coords = { -0.5f, 0.5f, 0.5f }, .texCoords = { s * 0, s * 1 } },

      //2: 4-7
      { .coords = { 0.5f, -0.5f, 0.5f }, .texCoords = { s * 1, s * 0 } },
      { .coords = { 0.5f, -0.5f, -0.5f }, .texCoords = { s * 2, s * 0 } },
      { .coords = { 0.5f, 0.5f, -0.5f }, .texCoords = { s * 2, s * 1 } },
      { .coords = { 0.5f, 0.5f, 0.5f }, .texCoords = { s * 1, s * 1 } },

      //3: 8-11
      { .coords = { -0.5f, 0.5f, 0.5f }, .texCoords = { s * 2, s * 0 } },
      { .coords = { 0.5f, 0.5f, 0.5f }, .texCoords = { s * 3, s * 0 } },
      { .coords = { 0.5f, 0.5f, -0.5f }, .texCoords = { s * 3, s * 1 } },
      { .coords = { -0.5f, 0.5f, -0.5f }, .texCoords = { s * 2, s * 1 } },

      //4: 12-15
      { .coords = { 0.5f, -0.5f, 0.5f }, .texCoords = { s * 0, s * 1 } },
      { .coords = { -0.5f, -0.5f, 0.5f }, .texCoords = { s * 1, s * 1 } },
      { .coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 1, s * 2 } },
      { .coords = { 0.5f, -0.5f, -0.5f }, .texCoords = { s * 0, s * 2 } },

      //5: 16-19
      { .coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 1, s * 1 } },
      { .coords = { -0.5f, -0.5f, 0.5f }, .texCoords = { s * 2, s * 1 } },
      { .coords = { -0.5f, 0.5f, 0.5f }, .texCoords = { s * 2, s * 2 } },
      { .coords = { -0.5f, 0.5f, -0.5f }, .texCoords = { s * 1, s * 2 } },

      //6: 20-23
      { .coords = { 0.5f, -0.5f, -0.5f }, .texCoords = { s * 2, s * 1 } },
      { .coords = { -0.5f, -0.5f, -0.5f }, .texCoords = { s * 3, s * 1 } },
      { .coords = { -0.5f, 0.5f, -0.5f }, .texCoords = { s * 3, s * 2 } },
      { .coords = { 0.5f, 0.5f, -0.5f }, .texCoords = { s * 2, s * 2 } }
   });

   vecPushStackArray(size_t, ibo,
   { 0, 1, 2, 0, 2, 3,
   4, 5, 6, 4, 6, 7,
   8, 9, 10, 8, 10, 11,
   12, 13, 14, 12, 14, 15,
   16, 17, 18, 16, 18, 19,
   20, 21, 22, 20, 22, 23 });
}
static void _createDie(DiceManager *self){
   Entity *e = entityCreate(self->system);
   COMPONENT_ADD(e, PositionComponent, 100, 275);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/d6.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, MeshComponent, self->vbo, self->ibo, 32, vNormalized((Float3){ 1.0f, 1.0f, 1.0f }), 0.0f);

   entityUpdate(e);
   self->die = e;
}

ImplManagerVTable(DiceManager)

DiceManager *createDiceManager(EntitySystem *system){
   DiceManager *out = checkedCalloc(1, sizeof(DiceManager));
   out->system = system;
   out->m.vTable = CreateManagerVTable(DiceManager);

   out->vbo = vecCreate(Vertex)(NULL);
   out->ibo = vecCreate(size_t)(NULL);

   _buildDiceBuffers(out->vbo, out->ibo);
   _createDie(out);

   return out;
}

void _destroy(DiceManager *self){
   vecDestroy(Vertex)(self->vbo);
   vecDestroy(size_t)(self->ibo);
   checkedFree(self);
}
void _onDestroy(DiceManager *self, Entity *e){}
void _onUpdate(DiceManager *self, Entity *e){}

void diceManagerUpdate(DiceManager *self){
   MeshComponent *mc = entityGet(MeshComponent)(self->die);
   PositionComponent  *pc = entityGet(PositionComponent)(self->die);
   Mouse *mouse = appGetMouse(appGet());
   //Int2 mousePos = mouseGetPosition(mouse);

   mc->angle = 3.1415926f * (float)(appGetTime(appGet()) / 1000.0f);
   //pc->x = mousePos.x;
   //pc->y = mousePos.y;

   // float angle = 
}