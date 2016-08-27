#include "LightGrid.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"

#include "Viewport.h"

#include <math.h>
#include "segautils/Math.h"
#include "segautils/StandardVectors.h"
#include "LightDebugger.h"
#include "WorldView.h"
#include "GridManager.h"

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

typedef struct  {
   size_t index;
   byte set;
}OcclusionMapEntry;

#define VectorT OcclusionMapEntry
#include "segautils/Vector_Create.h"

typedef struct LightGrid_t {
   WorldView *view;
   LightData grid[LIGHT_GRID_CELL_COUNT];
   OcclusionCell *occlusion;
   byte ambientLevel;
   vec(LightSourcePtr) *sources;

   vec(TileLights) *tileGrid;
   Int2 tileSize;
   size_t tileCount;
   vec(size_t) *usedLights;//used to stop double-lighting
   vec(OcclusionMapEntry) *occMap;
   Int2 vpPos;//set by update
   Recti lightArea;//for testing
}LightGrid;

LightGrid *lightGridCreate(WorldView *view) {
   LightGrid *out = checkedCalloc(1, sizeof(LightGrid));
   out->occlusion = checkedCalloc(LIGHT_GRID_CELL_COUNT, sizeof(OcclusionCell));
   out->view = view;

   out->sources = vecCreate(LightSourcePtr)(&lightSourcePtrDestroy);
   out->tileGrid = vecCreate(TileLights)(&tileLightsDestroy);
   out->usedLights = vecCreate(size_t)(NULL);
   out->occMap = vecCreate(OcclusionMapEntry)(NULL);
   return out;
}
void lightGridDestroy(LightGrid *self) {
   vecDestroy(LightSourcePtr)(self->sources);
   vecDestroy(TileLights(self->tileGrid));
   vecDestroy(size_t)(self->usedLights);
   vecDestroy(OcclusionMapEntry)(self->occMap);
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
      pos.x + adjRadius + 1,
      pos.y + adjRadius + 1
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
   GridManager *gm = self->view->gridManager;
   Tile *t = gridManagerTileAt(gm, tile);
   TileSchema *oldSchema = gridManagerGetSchema(gm, tileGetSchema(t));

   gridManagerXYFromCellID(gm, tile, &pos.x, &pos.y);

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
   self->AABB.right = x + adjRadius + 1;
   self->AABB.bottom = y + adjRadius + 1;
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
   LightGrid *lg;
}BlockCheckData;

static bool _lineIsBlocked(Int2 origin, Int2 target, bool occludingOrigin, LightGrid *lg) {
   int w = rectiWidth(&lg->lightArea);
   int dx = target.x - origin.x;
   int dy = target.y - origin.y;
   int steps = MAX(1, abs(dx) > abs(dy) ? abs(dx) : abs(dy));
   float xinc = dx / (float)steps;
   float yinc = dy / (float)steps;
   float fx = (float)origin.x, fy = (float)origin.y;
   float interx, intery;
   int i = 0;
   size_t currentTile = INF;
   static float iSize = 1.0f / GRID_CELL_SIZE;
   byte lastSet = 0;
   bool passed = false;

   //half-pixel :D
   fx += iSize * 0.5f;
   fy += iSize * 0.5f;

   for (i = 0; i < steps; ++i) {
      size_t t = 0;
      int ix, iy;
      fx += xinc;
      fy += yinc;

      interx = fx * iSize;
      intery = fy * iSize;

      //floating-truncation for negative coords needs to work in reverse!
      // -1.5 -> -2
      if (interx < 0.0f && origin.x < target.x) { interx = floorf(interx); }
      if (intery < 0.0f && origin.y < target.y) { intery = floorf(intery); }

      ix = (int)(interx);
      iy = (int)(intery);

      ix = ix + lg->vpPos.x - lg->lightArea.left;
      iy = iy + lg->vpPos.y - lg->lightArea.top;
      t = iy * w + ix;

      if (t != currentTile) {
         OcclusionMapEntry *ome = vecAt(OcclusionMapEntry)(lg->occMap, t);

         if (ome->index < INF) {//if the current tile occludes

            if (lastSet == 0) {//if its out first set weve reached
               lastSet = ome->set;//set it and continue
            }
            else if (lastSet != ome->set) {//if weve seen a set and this is different, we're blocked
               return true;
            }
         }
         else if (lastSet > 0) {//this tile doesnt occlude but weve been through a set(so we're pasing out the other end)

            if (occludingOrigin) {
               

               //only llow pass through once!
               if (passed) {
                  return true;
               }
               else {
                  passed = true;
                  //if the origin is occluding we can pass through the bottom (so only return otherwise
                  if (target.y <= (origin.y + GRID_CELL_SIZE)) {
                     return true;
                  }
               }

            }
            else {
               return true;
            }
         }
         currentTile = t;
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
#define FULLY_LIT_THRESHOLD 10
#define CHECK_COUNT 16

static void _buildOccRects(Int2 target, Int2 origin, Recti *originArea, Recti *targetArea, Int2 *orCenter, Int2 *tarCenter) {
   *originArea = (Recti){
      .left = (origin.x * GRID_CELL_SIZE),
      .top = (origin.y * GRID_CELL_SIZE),
      .right = (origin.x * GRID_CELL_SIZE + GRID_CELL_SIZE),
      .bottom = (origin.y * GRID_CELL_SIZE + GRID_CELL_SIZE)
   };

   *targetArea = (Recti){
      .left = (target.x * GRID_CELL_SIZE),
      .top = (target.y * GRID_CELL_SIZE),
      .right = (target.x * GRID_CELL_SIZE + GRID_CELL_SIZE),
      .bottom = (target.y * GRID_CELL_SIZE + GRID_CELL_SIZE)
   };

   *orCenter = (Int2){ originArea->left + (GRID_CELL_SIZE >> 1), originArea->top + (GRID_CELL_SIZE >> 1)};
   *tarCenter = (Int2){ targetArea->left + (GRID_CELL_SIZE >> 1), targetArea->top + (GRID_CELL_SIZE >> 1) };

   originArea->left += 1;
   originArea->top += 1;
   originArea->right -= 2;
   originArea->bottom -= 1;

   targetArea->left += 1;
   targetArea->top += 1;
   targetArea->right -= 1;
   targetArea->bottom -= 1;
}

static byte _calculateOcclusionOnPoint(byte calculatedLevel, Int2 target, Int2 origin, bool occludingOrigin, LightGrid *self) {
   static const float invertedThreshold = 1.0f / (float)FULLY_LIT_THRESHOLD;
   static const int minimumBlocksForShading = CHECK_COUNT - FULLY_LIT_THRESHOLD + 1;
   int occBlocks;
   Int2 orCenter, tarCenter;
   Recti originArea, targetArea;


   _buildOccRects(target, origin, &originArea, &targetArea, &orCenter, &tarCenter);

   occBlocks = 1;

   //occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.left, targetArea.top }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.right, targetArea.top }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.left, targetArea.bottom }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked(orCenter, (Int2) { targetArea.right, targetArea.bottom }, occludingOrigin, self);

   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.top }, tarCenter, occludingOrigin, self);

   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.top }, tarCenter, occludingOrigin, self);

   //short circuiting, less than 3 blockers here means there wont be enough blocking to dim the light
   //if (occBlocks < (minimumBlocksForShading - 8)) {
   //   return calculatedLevel;
   //}

   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, occludingOrigin, self);
   //occBlocks += _lineIsBlocked((Int2) { originArea.right, originArea.bottom }, tarCenter, occludingOrigin, self);

   //short circuiting, same here, the minimum number of unlblocked rays for a fully lit tile is 13
   //if (occBlocks < (minimumBlocksForShading - 4)) {
   //   return calculatedLevel;
   //}

   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, occludingOrigin, self);
   occBlocks += _lineIsBlocked((Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, occludingOrigin, self);

   //if (occBlocks < minimumBlocksForShading) {
   //   return calculatedLevel;
   //}

   return (byte)(calculatedLevel * ((CHECK_COUNT - occBlocks) * invertedThreshold));
}

static void addDebugRay(int *occBlocks, Int2 r1, Int2 r2, LightGrid *self) {
   LightDebugger * lb = self->view->lightDebugger;
   bool blocked = _lineIsBlocked(r1, r2, false, self);
   *occBlocks += blocked;
   lightDebuggerAddRay(lb, r1, r2, blocked);
}

void lightGridDebug(LightGrid *self, Int2 source, Int2 target) {

   static const float invertedThreshold = 1.0f / (float)FULLY_LIT_THRESHOLD;
   static const int minimumBlocksForShading = CHECK_COUNT - FULLY_LIT_THRESHOLD + 1;
   int occBlocks;
   Int2 orCenter, tarCenter;
   Recti originArea, targetArea;

   LightDebugger * lb = self->view->lightDebugger;

   
   _buildOccRects(target, source, &originArea, &targetArea, &orCenter, &tarCenter);
   lightDebuggerStartNewSet(lb, originArea, targetArea);

   occBlocks = 1;

   //addDebugRay(&occBlocks, orCenter, (Int2) { targetArea.left, targetArea.top }, self);
   //addDebugRay(&occBlocks, orCenter, (Int2) { targetArea.right, targetArea.top }, self);
   //addDebugRay(&occBlocks, orCenter, (Int2) { targetArea.left, targetArea.bottom }, self);
   //addDebugRay(&occBlocks, orCenter, (Int2) { targetArea.right, targetArea.bottom }, self);

   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, self);
   //addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.top }, tarCenter, self);

   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.top }, (Int2) { targetArea.left, targetArea.bottom }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.top }, (Int2) { targetArea.right, targetArea.bottom }, self);
   //addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.top }, tarCenter, self);

   //short circuiting, less than 3 blockers here means there wont be enough blocking to dim the light
   //if (occBlocks < (minimumBlocksForShading - 10)) {
   //   return;
   //}

   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, self);
   //addDebugRay(&occBlocks, (Int2) { originArea.right, originArea.bottom }, tarCenter, self);

   //short circuiting, same here, the minimum number of unlblocked rays for a fully lit tile is 13
   //if (occBlocks < (minimumBlocksForShading - 5)) {
   //   return;
   //}

   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.top }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.left, targetArea.bottom }, self);
   addDebugRay(&occBlocks, (Int2) { originArea.left, originArea.bottom }, (Int2) { targetArea.right, targetArea.bottom }, self);

   //if (occBlocks < minimumBlocksForShading) {
   //   return;
   //}

   return;
}


//returns nonzero if invalid or already processed
static int _processOcclusionNeighbors(LightGrid *self, Recti *area, int x, int y, byte set) {
   int w = rectiWidth(area);
   int h = rectiHeight(area);
   size_t index = y*w + x;
   OcclusionMapEntry *ome = NULL;

   if (x < 0 || x >= w || y < 0 || y >= h) {
      return 1;
   }

   ome = vecAt(OcclusionMapEntry)(self->occMap, index);
   if (ome->index == INF || ome->set != 0) {
      return 1;
   }

   ome->set = set;
   _processOcclusionNeighbors(self, area, x - 1, y, set);
   _processOcclusionNeighbors(self, area, x, y - 1, set);
   _processOcclusionNeighbors(self, area, x + 1, y, set);
   _processOcclusionNeighbors(self, area, x, y + 1, set);

   return 0;
}

static void _addPoint(LightGrid *self, PointLight light) {   
   Recti unboundedLightArea, lightArea; 
   int width, height;
   int adjRadius, r2, adjLevel;
   int occluderCount;
   int x, y;
   int i;
   float widthFactor = 0.0f;
   bool originOccludes = false;
   GridManager *gm = self->view->gridManager;

   {
      int i = (5 << 1) + 1;
      float r = (i*i) / 2.0f;
      
   }

   //bound the brightness (0 - max) and radius (0 - level/radius whichever's bigger)
   adjLevel = MIN(MAX(light.level, 0), MAX_BRIGHTNESS);
   adjRadius = MAX(0, MAX(light.radius, light.fadeWidth));

   if (light.fadeWidth > 0) {
      //inverse ratio of the fade width to the light level it needs to traverse
      widthFactor = adjLevel / (float)light.fadeWidth;
   }
   
   //add 0.5 to radius before squaring, this is for quick distance checks
   r2 = ((((adjRadius << 1) + 1) * ((adjRadius << 1) + 1)) >> 2) + 1;
   
   //create our area bounded within the vp
   unboundedLightArea = (Recti){
      .left =   light.origin.x - adjRadius,
      .top =    light.origin.y - adjRadius,
      .right =  light.origin.x + adjRadius + 1,
      .bottom = light.origin.y + adjRadius + 1
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
   //gridmanager will modify our rect to not go out of bounds
   occluderCount = gridManagerQueryOcclusion(gm, &unboundedLightArea, self->occlusion);
   self->lightArea = unboundedLightArea;
   if (occluderCount > 0) {
      int left = self->lightArea.left;
      int top = self->lightArea.top;
      int w = rectiWidth(&self->lightArea);
      int h = rectiHeight(&self->lightArea);
      byte currentSet = 1;
      vecClear(OcclusionMapEntry)(self->occMap);
      vecResize(OcclusionMapEntry)(self->occMap, w * h, &(OcclusionMapEntry){INF, 0});

      for (i = 0; i < occluderCount; ++i) {
         OcclusionCell *oc = self->occlusion + i;
         oc->area = (Recti) {
            .left = ((oc->x * GRID_CELL_SIZE) << 1),
            .top = ((oc->y * GRID_CELL_SIZE) << 1),
            .right = ((oc->x * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1),
            .bottom = ((oc->y * GRID_CELL_SIZE + GRID_CELL_SIZE) << 1)
         };

         //now save the indices into our occArray into their correct places inside the occMap    
         {
            int occX = oc->wx - left;
            int occY = oc->wy - top;
            size_t occMapIndex = occY * w + occX;

            if (oc->wx - self->vpPos.x == light.origin.x &&
               oc->wy - self->vpPos.y == light.origin.y) {
               //origin occludes!
               originOccludes = true;
            }

            vecAt(OcclusionMapEntry)(self->occMap, occMapIndex)->index = i;
         }   
      }

      for (i = 0; i < occluderCount; ++i) {
         OcclusionCell *oc = self->occlusion + i;
         int occX = oc->wx - left;
         int occY = oc->wy - top;
         if (!_processOcclusionNeighbors(self, &self->lightArea, occX, occY, currentSet)) {
            ++currentSet;
         }
      }
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
                  originOccludes,
                  self); //the occluder data
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
   GridManager *gm = self->view->gridManager;
   Tile *t = gridManagerTileAt(gm, tile);
   Int2 tpos = { 0 };
   TileSchema *schema = gridManagerGetSchema(gm, tileGetSchema(t));

   gridManagerXYFromCellID(gm, tile, &tpos.x, &tpos.y);

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
   
   Recti worldGrid = { vpx, vpy, vpx + LIGHT_GRID_WIDTH , vpy + LIGHT_GRID_HEIGHT };
   Recti tileGrid = _tileGridAABB(self, &worldGrid);
   int x, y;

   self->vpPos.x = vpx;
   self->vpPos.y = vpy;

   //ambient lights/reset
   memset(self->grid, 0, sizeof(self->grid));   
   for (i = 0; i < LIGHT_GRID_CELL_COUNT; ++i) {
      self->grid[i].level = self->ambientLevel;
   }

   //add lightsd from tiles
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

void testLightRender(LightGrid *self, Frame *frame, Viewport *vp) {
   int x, y;
   for (y = self->lightArea.top; y <= self->lightArea.bottom; ++y) {
      for (x = self->lightArea.left; x <= self->lightArea.right; ++x) {
         int rX = x * GRID_CELL_SIZE - vp->worldPos.x;
         int rY = y * GRID_CELL_SIZE - vp->worldPos.y;
         frameRenderRect(frame, &vp->region, rX, rY, rX + GRID_CELL_SIZE, rY + GRID_CELL_SIZE, 5);
      }
   }
}

