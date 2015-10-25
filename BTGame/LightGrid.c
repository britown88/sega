#include "LightGrid.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"

#include "Viewport.h"
#include "Entities/Entities.h"
#include "CoreComponents.h"

#include <math.h>
#include "segautils/Math.h"

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
   OcclusionCell *occlusion;
   GridManager *parent;
}LightGrid;

LightGrid *lightGridCreate(GridManager *parent) {
   LightGrid *out = checkedCalloc(1, sizeof(LightGrid));
   out->occlusion = checkedCalloc(LIGHT_GRID_CELL_COUNT, sizeof(OcclusionCell));
   out->parent = parent;
   return out;
}
void lightGridDestroy(LightGrid *self) {
   checkedFree(self->occlusion);
   checkedFree(self);
}

typedef struct {
   Int2 origin;
   byte radius;
   byte level;
}PointLight;

typedef struct  {
   Int2 targetCell;
   bool occludedTarget;
   OcclusionCell *oList;
   int oCount;
}BlockCheckData;

static bool _lineIsBlocked(Int2 origin, Int2 target, BlockCheckData *data) {
   int i;
   for (i = 0; i < data->oCount; ++i) {
      OcclusionCell *oc = data->oList + i;

      if (lineSegmentIntersectsAABBi(origin, target, &oc->area)) {
         if (data->occludedTarget/* && abs(oc->x - data->targetCell.x) + abs(oc->y - data->targetCell.y) < 2*/) {
            continue;
         }
         return true;
      }
   }

   return false;
}

static byte _calculateOcclusionOnPoint(byte calculatedLevel, Int2 target, Int2 origin, OcclusionCell *oList, int oCount ) {
   static const float invertedThreshold = 1.0f / 13.0f;
   int i;
   //build our rects... shifting by 1 for precision
   Recti originArea = {
      .left = (origin.x * GRID_CELL_SIZE) << 1,
      .top = (origin.y * GRID_CELL_SIZE) << 1,
      .right = (origin.x * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1,
      .bottom = (origin.y * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1
   };

   Recti targetArea = {
      .left = (target.x * GRID_CELL_SIZE) << 1,
      .top = (target.y * GRID_CELL_SIZE) << 1,
      .right = (target.x * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1,
      .bottom = (target.y * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1
   };   

   Int2 orCenter = { originArea.left + GRID_CELL_SIZE, originArea.top + GRID_CELL_SIZE };
   Int2 tarCenter = { targetArea.left + GRID_CELL_SIZE, targetArea.top + GRID_CELL_SIZE };
   int occBlocks;

   BlockCheckData checkData = { {target.x, target.y}, false, oList, oCount };

   for (i = 0; i < oCount; ++i) {
      OcclusionCell *oc = oList + i;
      if (oc->x == target.x && oc->y == target.y) {
         checkData.occludedTarget = true;
         break;
      }
   }

   if (!_lineIsBlocked(orCenter, tarCenter, &checkData)) {
      return calculatedLevel;
   }

   originArea.left += 1;
   originArea.top += 1;
   originArea.right -= 1;
   originArea.bottom -= 1;

   targetArea.left += 1;
   targetArea.top += 1;
   targetArea.right -= 1;
   targetArea.bottom -= 1;

   occBlocks = 1;

   occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.right, targetArea.bottom }, &checkData);

   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, tarCenter, &checkData);

   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, tarCenter, &checkData);

   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, tarCenter, &checkData);

   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, tarCenter, &checkData);

   return (byte)(calculatedLevel * ((25 - occBlocks) * invertedThreshold));
   //return MIN(calculatedLevel, (25 - occBlocks) / 6);
}

static void _addPoint(LightGrid *self, PointLight light) {   
   Recti unboundedLightArea, lightArea; 
   int width, height;
   int adjRadius, r2, adjLevel;
   int occluderCount;
   int x, y;
   int i;

   //bound the brightness (0 - max) and radius (0 - level/radius whichever's bigger)
   adjLevel = MIN(MAX(light.level, 0), MAX_BRIGHTNESS);
   adjRadius = MAX(0, MAX(light.radius, adjLevel));
   r2 = adjRadius * adjRadius;
   
   //create our area bounded within the vp
   unboundedLightArea = (Recti){
      .left =   light.origin.x - adjRadius,
      .top =    light.origin.y - adjRadius,
      .right =  light.origin.x + adjRadius,
      .bottom = light.origin.y + adjRadius
   };

   lightArea = (Recti){
      .left =   MIN(LIGHT_GRID_WIDTH - 1, MAX(0, unboundedLightArea.left)),
      .top =    MIN(LIGHT_GRID_HEIGHT - 1, MAX(0, unboundedLightArea.top)),
      .right =  MIN(LIGHT_GRID_WIDTH - 1, MAX(0, unboundedLightArea.right)),
      .bottom = MIN(LIGHT_GRID_HEIGHT - 1, MAX(0, unboundedLightArea.bottom))
   };

   width = rectiWidth(&lightArea);
   height = rectiHeight(&lightArea);

   //if our area has no size, its off screen and can be ignored
   if(!width && !height){
      return;
   }

   //get our occlusion list and generate their rects
   occluderCount = gridManagerQueryOcclusion(self->parent, &unboundedLightArea, self->occlusion);
   for (i = 0; i < occluderCount; ++i) {
      OcclusionCell *oc = self->occlusion + i;
      oc->area = (Recti){
         .left = ((oc->x * GRID_CELL_SIZE) << 1),
         .top = ((oc->y * GRID_CELL_SIZE) << 1),
         .right = ((oc->x * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1),
         .bottom = ((oc->y * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1)
      };
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

            //calculate occlusion!
            calculatedLevel = _calculateOcclusionOnPoint(
               calculatedLevel, //starting level
               (Int2) { x, y }, //cell pos
               (Int2) { light.origin.x , light.origin.y }, //light's origin
               self->occlusion, occluderCount); //the occluder data

            //all done, add it in
            lightGridAt(self, x, y)->level += calculatedLevel;
         }
      }
   }
}

void lightGridUpdate(LightGrid *self, EntitySystem *es, short vpx, short vpy) {
   memset(self->grid, 0, sizeof(self->grid));

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

