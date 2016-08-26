#include "LightGrid.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"

#include "Viewport.h"

#include <math.h>
#include "segautils/Math.h"
#include "segautils/StandardVectors.h"

struct LightData_t {
   byte level;
   byte flags;
   bool tileLightAdded;
};

byte lightDataGetLevel(LightData *self) {
   return self->level;
}

#define TILE_GRID_SIZE 8 //height and width of a block of tilesd for purpose of light storage

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

struct LightSource_t {
   LightGrid *parent;
   LightSourceParams params;
   Int2 pos;
   Recti AABB;
};

typedef LightSource *LightSourcePtr;
#define VectorT LightSourcePtr
#include "segautils/Vector_Create.h"

void lightSourcePtrDestroy(LightSourcePtr *self) {
   checkedFree(*self);
}

typedef struct {
   vec(size_t) *tiles;//can be null if nothings in it!!
}TileLights;

#define VectorT TileLights
#include "segautils/Vector_Create.h"

void tileLightsDestroy(TileLights *self) {
   if (self->tiles) {
      vecDestroy(size_t)(self->tiles);
   }
}

typedef struct LightGrid_t {
   LightData grid[LIGHT_GRID_CELL_COUNT];
   OcclusionCell *occlusion;
   GridManager *parent;
   byte ambientLevel;
   vec(LightSourcePtr) *sources;

   vec(TileLights) *tileGrid;
   Int2 tileSize;
   size_t tileCount;
   vec(size_t) *usedLights;//used to stop double-lighting
}LightGrid;

LightGrid *lightGridCreate(GridManager *parent) {
   LightGrid *out = checkedCalloc(1, sizeof(LightGrid));
   out->occlusion = checkedCalloc(LIGHT_GRID_CELL_COUNT, sizeof(OcclusionCell));
   out->parent = parent;

   out->sources = vecCreate(LightSourcePtr)(&lightSourcePtrDestroy);
   out->tileGrid = vecCreate(TileLights)(&tileLightsDestroy);
   out->usedLights = vecCreate(size_t)(NULL);
   return out;
}
void lightGridDestroy(LightGrid *self) {
   vecDestroy(LightSourcePtr)(self->sources);
   vecDestroy(TileLights(self->tileGrid));
   vecDestroy(size_t)(self->usedLights);
   checkedFree(self->occlusion);
   checkedFree(self);
}

void lightGridLoadMap(LightGrid *self, int width, int height) {
   vecClear(TileLights(self->tileGrid));

   self->tileSize.x = width / TILE_GRID_SIZE + (width%TILE_GRID_SIZE > 0 ? 1 : 0);
   self->tileSize.y = height / TILE_GRID_SIZE + (height%TILE_GRID_SIZE > 0 ? 1 : 0);
   self->tileCount = self->tileSize.x * self->tileSize.y;

   vecResize(TileLights)(self->tileGrid, self->tileCount, &(TileLights){0});
}

static Recti _tileAABB(TileSchema *schema, Int2 pos) {
   int adjRadius = MAX(0, MAX(schema->radius, schema->fadeWidth));
   return (Recti) {
      pos.x - adjRadius,
      pos.y - adjRadius,
      pos.x + adjRadius,
      pos.y + adjRadius
   };
}

static Recti _tileGridAABB(LightGrid *self, const Recti *tileaabb) {
   return (Recti) {
      MAX(0, tileaabb->left / TILE_GRID_SIZE),
      MAX(0, tileaabb->top / TILE_GRID_SIZE),
      MIN(self->tileSize.x - 1, tileaabb->right / TILE_GRID_SIZE),
      MIN(self->tileSize.y - 1, tileaabb->bottom / TILE_GRID_SIZE)
   };
}

static size_t _tileGridIndex(LightGrid *self, Int2 pos) {
   size_t out = pos.y * self->tileSize.x + pos.x;
   if (out >= self->tileCount) {
      return INF;
   }
   return out;
}

void lightGridChangeTileSchema(LightGrid *self, size_t tile, TileSchema *schema) {
   Int2 pos = { 0 };
   Tile *t = gridManagerTileAt(self->parent, tile);
   TileSchema *oldSchema = gridManagerGetSchema(self->parent, tileGetSchema(t));

   gridManagerXYFromCellID(self->parent, tile, &pos.x, &pos.y);

   //remove the old tile from the light grid table
   if (oldSchema && oldSchema->lit) {      
      Recti taabb = _tileAABB(oldSchema, pos);
      Recti aabb = _tileGridAABB(self, &taabb);
      int x, y;

      for (y = aabb.top; y <= aabb.bottom; ++y) {
         for (x = aabb.left; x <= aabb.right; ++x) {
            size_t index = _tileGridIndex(self, (Int2) { x, y });
            if (index < INF) {
               TileLights *tl = vecAt(TileLights)(self->tileGrid, index);
               if (tl->tiles) {
                  vecRemove(size_t)(tl->tiles, &tile);
               }
            }
         }
      }
   }

   //and add the new one
   if (schema && schema->lit) {
      Recti taabb = _tileAABB(schema, pos);
      Recti aabb = _tileGridAABB(self, &taabb);
      int x, y;

      for (y = aabb.top; y <= aabb.bottom; ++y) {
         for (x = aabb.left; x <= aabb.right; ++x) {
            size_t index = _tileGridIndex(self, (Int2) { x, y });
            if (index < INF) {
               TileLights *tl = vecAt(TileLights)(self->tileGrid, index);
               if (!tl->tiles) {
                  tl->tiles = vecCreate(size_t)(NULL);
               }
               vecPushBack(size_t)(tl->tiles, &tile);
            }
         }
      }
   }
}

LightSource *lightGridCreateLightSource(LightGrid *self) {
   LightSource *out = checkedCalloc(1, sizeof(LightSource));
   out->parent = self;
   out->params.on = true;
   vecPushBack(LightSourcePtr)(self->sources, &out);
   return out;
}

static void _updateLightSourceAABB(LightSource *self) {
   int x = self->pos.x / GRID_CELL_SIZE;
   int y = self->pos.y / GRID_CELL_SIZE;
   int adjRadius = MAX(0, MAX(self->params.radius, self->params.fadeWidth));

   self->AABB.left = x - adjRadius;
   self->AABB.top = y - adjRadius;
   self->AABB.right = x + adjRadius;
   self->AABB.bottom = y + adjRadius;
}

void lightSourceSetParams(LightSource *self, LightSourceParams params) {
   self->params = params;
   _updateLightSourceAABB(self);
}
void lightSourceSetPosition(LightSource *self, Int2 pos) {
   self->pos = pos;
   _updateLightSourceAABB(self);
}
void lightSourceDestroy(LightSource *self) {
   vecRemove(LightSourcePtr)(self->parent->sources, &self);
}

typedef struct {
   Int2 origin;
   byte radius;
   byte level;
   byte fadeWidth;
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
         if (data->occludedTarget && abs(data->targetCell.x - oc->x) + abs(data->targetCell.y - oc->y) < 1) {
            continue;
         }
         return true;
      }
   }

   return false;
}

/* 
The light level of a partially-blocked tile is startingLevel * (UnblockedRayCount / FULLY_LIT_THRESHOLD)
This means that, of the 25 ray checks, the minimum number of unblkocked rays necessary for the tile to be fully lit is FULLY_LIT_THRESHOLD
any less than (25 - FULLY_LIT_THRESHOLD + 1) _blocked_ rays will result in no darkening, which can be used to shortcircuit the 
counts if enough blocked rays havent been found by given intervals
Also, if the target is an occluder itself, it only needs to verify that a single edge is lit (defined by 4 of the 5 rays hitting each of two corners)
*/
#define FULLY_LIT_THRESHOLD 13

static byte _calculateOcclusionOnPoint(byte calculatedLevel, Int2 target, Int2 origin, OcclusionCell *oList, int oCount ) {
   static const float invertedThreshold = 1.0f / (float)FULLY_LIT_THRESHOLD;
   static const int minimumBlocksForShading = 25 - FULLY_LIT_THRESHOLD + 1;
   static const int cornerBlockedThreshold = 4;

   int i;
   int lopLeftBlocked = 0, topRightBlocked = 0, bottomRightBlocked = 0, bottomLeftBlocked = 0;
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

   lopLeftBlocked = occBlocks;
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, tarCenter, &checkData);
   lopLeftBlocked = occBlocks - lopLeftBlocked;

   topRightBlocked = occBlocks;
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, tarCenter, &checkData);
   topRightBlocked = occBlocks - topRightBlocked;

   //short circuiting, less than 3 blockers here means there wont be enough blocking to dim the light
   if (occBlocks < (minimumBlocksForShading - 10) || (checkData.occludedTarget && lopLeftBlocked <= cornerBlockedThreshold && topRightBlocked <= cornerBlockedThreshold)) {
      return calculatedLevel;
   }

   bottomRightBlocked = occBlocks;
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, tarCenter, &checkData);
   bottomRightBlocked = occBlocks - bottomRightBlocked;

   //short circuiting, same here, the minimum number of unlblocked rays for a fully lit tile is 13
   if (occBlocks < (minimumBlocksForShading - 5) || (checkData.occludedTarget && bottomRightBlocked <= cornerBlockedThreshold && topRightBlocked <= cornerBlockedThreshold)) {
      return calculatedLevel;
   }

   bottomLeftBlocked = occBlocks;
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, &checkData);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, tarCenter, &checkData);
   bottomLeftBlocked = occBlocks - bottomLeftBlocked;

   if (occBlocks < minimumBlocksForShading) {
      return calculatedLevel;
   }

   if (checkData.occludedTarget) {
      if ((lopLeftBlocked <= cornerBlockedThreshold || bottomRightBlocked <= cornerBlockedThreshold) &&
          (topRightBlocked <= cornerBlockedThreshold || bottomLeftBlocked <= cornerBlockedThreshold)) {
         return calculatedLevel;
      }
   }

   return (byte)(calculatedLevel * ((25 - occBlocks) * invertedThreshold));
}

static void _addPoint(LightGrid *self, PointLight light) {   
   Recti unboundedLightArea, lightArea; 
   int width, height;
   int adjRadius, r2, adjLevel;
   int occluderCount;
   int x, y;
   int i;
   float widthFactor = 0.0f;

   //bound the brightness (0 - max) and radius (0 - level/radius whichever's bigger)
   adjLevel = MIN(MAX(light.level, 0), MAX_BRIGHTNESS);
   adjRadius = MAX(0, MAX(light.radius, light.fadeWidth));

   if (light.fadeWidth > 0) {
      //inverse ratio of the fade width to the light level it needs to traverse
      widthFactor = adjLevel / (float)light.fadeWidth;
   }
   
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
            byte calculatedLevel = widthFactor > 0 ? (byte)((adjRadius - dist) * widthFactor) : adjLevel;

            //calculate occlusion!
            if (occluderCount > 0) {
               calculatedLevel = _calculateOcclusionOnPoint(
                  calculatedLevel, //starting level
                  (Int2) { x, y }, //cell pos
                  (Int2) { light.origin.x , light.origin.y }, //light's origin
                  self->occlusion, occluderCount); //the occluder data
            }
            

            if (calculatedLevel) {
               lightGridAt(self, x, y)->flags |= LIGHTFLAGS_DIRECTLYLIT;
            }

            //all done, add it in
            lightGridAt(self, x, y)->level += calculatedLevel;
         }
      }
   }
}

static void _addTileLight(LightGrid *self, size_t tile, short vpx, short vpy) {
   Tile *t = gridManagerTileAt(self->parent, tile);
   Int2 tpos = { 0 };
   TileSchema *schema = gridManagerGetSchema(self->parent, tileGetSchema(t));

   gridManagerXYFromCellID(self->parent, tile, &tpos.x, &tpos.y);

   if (schema->lit) {
      PointLight p = { 0 };
      p.origin = (Int2) {
         .x = tpos.x - vpx,
         .y = tpos.y - vpy
      };
      p.radius = schema->radius;
      p.level = schema->centerLevel;
      p.fadeWidth = schema->fadeWidth;

      _addPoint(self, p); 
   }
}

void lightGridUpdate(LightGrid *self, short vpx, short vpy) {
   int i = 0;
   memset(self->grid, 0, sizeof(self->grid));
   Recti worldGrid = { vpx, vpy, vpx + LIGHT_GRID_WIDTH , vpy + LIGHT_GRID_HEIGHT };
   Recti tileGrid = _tileGridAABB(self, &worldGrid);
   int x, y;

   for (i = 0; i < LIGHT_GRID_CELL_COUNT; ++i) {
      self->grid[i].level = self->ambientLevel;
   }

   vecClear(size_t)(self->usedLights);

   for (y = tileGrid.top; y <= tileGrid.bottom; ++y) {
      for (x = tileGrid.left; x <= tileGrid.right; ++x) {
         size_t index = _tileGridIndex(self, (Int2) { x, y });
         if (index < INF) {
            TileLights *tl = vecAt(TileLights)(self->tileGrid, index);
            if (tl->tiles) {
               vecForEach(size_t, i, tl->tiles, {
                  if (vecIndexOf(size_t)(self->usedLights, i) == INF) {
                     _addTileLight(self, *i, vpx, vpy);
                     vecPushBack(size_t)(self->usedLights, i);
                  }
               });
            }
         }
      }
   }

   //add tile lights
   //for (i = 0; i < LIGHT_GRID_CELL_COUNT; ++i) {
   //   int x = (i % LIGHT_GRID_WIDTH) + vpx;
   //   int y = (i / LIGHT_GRID_WIDTH) + vpy;
   //   size_t index = gridManagerCellIDFromXY(self->parent, x, y);

   //   if (index < INF) {
   //      Tile *t = gridManagerTileAtXY(self->parent, x, y);
   //      TileSchema *schema = gridManagerGetSchema(self->parent, tileGetSchema(t));
   //      if (schema->lit) {
   //         _addPoint(self, (PointLight) {
   //            .origin = {
   //               .x = x - vpx,
   //               .y = y - vpy
   //            },
   //               .radius = schema->radius,
   //               .level = schema->centerLevel,
   //               .fadeWidth = schema->fadeWidth
   //         });
   //      }
   //   }
   //}

   //add actor lights (from lightsources)
   vecForEach(LightSourcePtr, src, self->sources, {
      if (rectiIntersects(worldGrid, (*src)->AABB)) {
         LightSourceParams *lsp = &(*src)->params;
         Int2 *lsPos = &(*src)->pos;
         _addPoint(self, (PointLight) {
            .origin = {
               .x = ((lsPos->x + (GRID_CELL_SIZE >> 1)) / GRID_CELL_SIZE) - vpx,
               .y = ((lsPos->y + (GRID_CELL_SIZE >> 1)) / GRID_CELL_SIZE) - vpy
            },
               .radius = lsp->radius,
               .level = lsp->centerLevel,
               .fadeWidth = lsp->fadeWidth
         });
      }
   });
}

void lightGridSetAmbientLight(LightGrid *self, byte level) {
   self->ambientLevel = level;
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

