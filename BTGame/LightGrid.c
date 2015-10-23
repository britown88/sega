#include "LightGrid.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"

#include "Viewport.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"

#include <math.h>

#pragma region MASKS

static const byte LightMasks[LIGHT_LEVEL_COUNT][16] =
//0
{{    1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1 },
//1
{     0, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 1,
      1, 1, 1, 0 },
//2
{     0, 1, 1, 1,
      1, 1, 0, 1,
      1, 0, 1, 1,
      1, 1, 1, 0 },
//3
{     1, 0, 1, 0,
      0, 1, 0, 1,
      1, 0, 1, 0,
      0, 1, 0, 1 },
//4
{     1, 0, 0, 0,
      0, 0, 1, 0,
      0, 1, 0, 0,
      0, 0, 0, 1 },
//5
{     1, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 1 },
//6
{     0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0 }};

#pragma endregion

typedef struct LightGrid_t {
   LightData grid[LIGHT_GRID_CELL_COUNT];
}LightGrid;

LightGrid *lightGridCreate() {
   return checkedCalloc(1, sizeof(LightGrid));
}
void lightGridDestroy(LightGrid *self) {
   checkedFree(self);
}

static void _calculateLight(LightGrid *self, int ox, int oy, byte radius, byte centerLevel) {
   int x, y;
   centerLevel = MIN(MAX(centerLevel, 0), MAX_BRIGHTNESS);
   radius = MAX(radius, centerLevel);
   for (y = -radius; y <= radius; ++y) {
      for (x = -radius; x <= radius; ++x) {
         if (ox + x >= 0 && ox + x < LIGHT_GRID_WIDTH &&
            oy + y >= 0 && oy + y < LIGHT_GRID_HEIGHT) {
            int xxyy = x*x + y*y;
            if (xxyy <= radius * radius) {
               int dist = MAX(0, (int)sqrtf((float)xxyy));

               lightGridAt(self, ox + x, oy + y)->level += MIN(centerLevel, radius - dist);
            }
         }
      }
   }
}


static void _updateLightEntity(LightGrid *self, EntitySystem *es, byte vpx, byte vpy, LightComponent *lc) {
   Entity *e = componentGetParent(lc, es);
   PositionComponent *pc = entityGet(PositionComponent)(e);
   _calculateLight(self, (pc->x / GRID_CELL_SIZE) - vpx, (pc->y / GRID_CELL_SIZE) - vpy, lc->radius, lc->centerLevel);
}

void lightGridUpdate(LightGrid *self, EntitySystem *es, byte vpx, byte vpy) {
   memset(self->grid, 0, sizeof(self->grid));

   COMPONENT_QUERY(es, LightComponent, lc, {
      _updateLightEntity(self, es, vpx, vpy, lc);
   });

}

LightData *lightGridAt(LightGrid *self, byte x, byte y) {
   if (x < 0 || x >= LIGHT_GRID_WIDTH || y < 0 || y >= LIGHT_GRID_HEIGHT) {
      return NULL;
   }

   return self->grid + (y * LIGHT_GRID_WIDTH + x);
}

void lightDataRender(LightData *light, Frame *frame, FrameRegion *vp, short x, short y) {
   short shadex, shadey;
   if (!light || light->level >= MAX_BRIGHTNESS) {
      return;
   }

   for (shadey = 0; shadey < GRID_CELL_SIZE; ++shadey) {
      for (shadex = 0; shadex < GRID_CELL_SIZE; ++shadex) {
         byte maskX = shadex % 4, maskY = shadey % 4;
         byte maskIndex = maskY * 4 + maskX;

         if (LightMasks[light->level][maskIndex]) {
            frameRenderPoint(frame, vp, x + shadex, y + shadey, 0);
         }
      }
   }
}

