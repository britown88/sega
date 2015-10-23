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

static byte _calculateSingleOccluder(byte calculatedLevel, Recti *origin, Recti *target, OcclusionCell *cell) {
   Recti occludedArea = {
      .left = (cell->x * GRID_CELL_SIZE) << 1,
      .top = (cell->y * GRID_CELL_SIZE) << 1,
      .right = (cell->x * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1,
      .bottom = (cell->y * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1
   };

   Int2 orCenter = { origin->left + GRID_CELL_SIZE, origin->top + GRID_CELL_SIZE };
   Int2 tarCenter = { target->left + GRID_CELL_SIZE, target->top + GRID_CELL_SIZE };
   int occBlocks;

   if (!lineSegmentIntersectsAABBi(orCenter, tarCenter, &occludedArea)) {
      //cente risnt blocked, full light
      return calculatedLevel;
   }

   //inset the origin a bit
   origin->left -= 1;
   origin->top -= 1;
   origin->right += 1;
   origin->bottom += 1;

   target->left += 1;
   target->top += 1;
   target->right -= 1;
   target->bottom -= 1;

   occBlocks = 1;

   occBlocks += lineSegmentIntersectsAABBi(orCenter, (Int2) { target->left, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi(orCenter, (Int2) { target->right, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi(orCenter, (Int2) { target->left, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi(orCenter, (Int2) { target->right, target->bottom }, &occludedArea);

   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->top }, (Int2) { target->left, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->top }, (Int2) { target->right, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->top }, (Int2) { target->left, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->top }, (Int2) { target->right, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->top }, tarCenter, &occludedArea);

   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->top }, (Int2) { target->left, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->top }, (Int2) { target->right, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->top }, (Int2) { target->left, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->top }, (Int2) { target->right, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->top }, tarCenter, &occludedArea);

   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->bottom }, (Int2) { target->left, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->bottom }, (Int2) { target->right, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->bottom }, (Int2) { target->left, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->bottom }, (Int2) { target->right, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->right, origin->bottom }, tarCenter, &occludedArea);

   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->bottom }, (Int2) { target->left, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->bottom }, (Int2) { target->right, target->top }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->bottom }, (Int2) { target->left, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->bottom }, (Int2) { target->right, target->bottom }, &occludedArea);
   occBlocks += lineSegmentIntersectsAABBi((Int2) { origin->left, origin->bottom }, tarCenter, &occludedArea);

   //at this point occBlocks out of 25 rays were blocked from the target
   //use this ratio to determine resultant brightness
   return MIN(calculatedLevel, (25 - occBlocks) / 4);
}

static byte _calculateOcclusionOnPoint(byte calculatedLevel, Int2 target, Int2 origin, OcclusionCell *oList, int oCount ) {
   int i;
   byte out = calculatedLevel;
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

   for (i = 0; i < oCount; ++i) {
      OcclusionCell *oc = oList + i;
      if (oc->x == target.x && oc->y == target.y) {
         //occluder lies on the cell we're lighting, ignore it
         return calculatedLevel;
      }

      // now we do ray vs aabb collision tests on the lighting 
      // square vs the occluder and count up unblocked rays
      out = _calculateSingleOccluder(out, &originArea, &targetArea, oc);
   }

   return out;
}

static void _addPoint(LightGrid *self, PointLight light) {   
   Recti lightArea; 
   int width, height;
   int adjRadius, r2, adjLevel;
   int occluderCount;
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

   width = rectiWidth(&lightArea);
   height = rectiHeight(&lightArea);

   //if our area has no size, its off screen and can be ignored
   if(!width && !height){
      return;
   }

   occluderCount = gridManagerQueryOcclusion(self->parent, &lightArea, self->occlusion);

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

