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
   OcclusionCell occlusion[LIGHT_GRID_CELL_COUNT];
   GridManager *parent;
}LightGrid;

LightGrid *lightGridCreate(GridManager *parent) {
   LightGrid *out = checkedCalloc(1, sizeof(LightGrid));
   out->parent = parent;
   return out;
}
void lightGridDestroy(LightGrid *self) {
   checkedFree(self);
}

typedef struct {
   Int2 origin;
   byte radius;
   byte level;
}PointLight;

static void _addPoint(LightGrid *self, PointLight light) {   
   Recti lightArea;    
   int adjRadius, r2, adjLevel;
   int x, y;

   //bound the brightness (0 - max) and radius (0 - level/radius whichever's bigger)
   adjLevel = MIN(MAX(light.level, 0), MAX_BRIGHTNESS);
   adjRadius = MAX(0, MAX(light.radius, adjLevel));
   r2 = adjRadius * adjRadius;
   
   //create our area bounded within the vp
   lightArea = (Recti){
      .left =   MIN(LIGHT_GRID_WIDTH - 1, MAX(0, light.origin.x - adjRadius)),
      .top =    MIN(LIGHT_GRID_HEIGHT - 1, MAX(0, light.origin.y - adjRadius)),
      .right =  MIN(LIGHT_GRID_WIDTH - 1, MAX(0, light.origin.x + adjRadius)),
      .bottom = MIN(LIGHT_GRID_HEIGHT - 1, MAX(0, light.origin.y + adjRadius))
   };

   //if our area has no size, its off screen and can be ignored
   if(!rectiWidth(&lightArea) && !rectiHeight(&lightArea)){
      return;
   }

   //brute force the whole area, we can square-check each tile without having to sqrt a bunch
   for (y = lightArea.top; y <= lightArea.bottom; ++y) {
      int yminuso = y - light.origin.y;
      for (x = lightArea.left; x <= lightArea.right; ++x) {
         int xminuso = x - light.origin.x;
         int xxyy = xminuso*xminuso + yminuso*yminuso;

         //only calc distance if we're squarely inside the radius
         if (xxyy <= r2) {
            int dist = MAX(0, (int)sqrtf((float)xxyy));
            byte calculatedLevel = MIN(adjLevel, adjRadius - dist);

            //all done, add it in
            lightGridAt(self, x, y)->level += calculatedLevel;
         }
      }
   }
}

void lightGridUpdate(LightGrid *self, EntitySystem *es, byte vpx, byte vpy) {
   memset(self->grid, 0, sizeof(self->grid));
   memset(self->occlusion, 0, sizeof(self->occlusion));

   gridManagerQueryOcclusion(self->parent, self->occlusion);

   COMPONENT_QUERY(es, LightComponent, lc, {
      Entity *e = componentGetParent(lc, es);
      PositionComponent *pc = entityGet(PositionComponent)(e);

      _addPoint(self, (PointLight) {
            .origin = {
               .x = (pc->x / GRID_CELL_SIZE) - vpx,
               .y = (pc->y / GRID_CELL_SIZE) - vpy
            },
            .radius = lc->radius,
            .level = lc->centerLevel
      });
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

