#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "GridManager.h"
#include "ImageLibrary.h"
#include "LightGrid.h"
#include "GameHelpers.h"
#include "Map.h"
#include "Sprites.h"
#include "assets.h"
#include <stdlib.h>

#define SCHEMA_COUNT 256
#define PARTITION_SIZE 16

typedef struct {
   vec(ActorPtr) *actors;
   size_t index;
}Partition;


static void _partitionDestroy(Partition *p) {
   if (p->actors) {
      vecDestroy(ActorPtr)(p->actors);
   }
}

#define VectorT Partition
#include "segautils/Vector_Create.h"

#define VectorT TileSchema
#include "segautils/Vector_Create.h"


struct GridToken_t {
   GridManager *parent;
   Actor *owner;
   vec(size_t) *occupyingPartitions;
};

typedef GridToken *GridTokenPtr;
#define VectorT GridTokenPtr
#include "segautils/Vector_Create.h"

void gridTokenPtrDestroy(GridTokenPtr *self) {
   vecDestroy(size_t)((*self)->occupyingPartitions);
   checkedFree(*self);
}

struct GridManager_t {
   WorldView *view;

   vec(GridTokenPtr) *tokens;
   bool lightMode;

   //The tile atlas
   ManagedImage *tilePalette;

   //the schema table
   vec(TileSchema) *schemas;

   //the actual grid
   short height, width;
   size_t cellCount;
   LightGrid *lightGrid;
   Map *map;

   // entities needing drawing this frame (ref to this is returned)
   vec(ActorPtr) *inViewActors;

   // actor partition members
   vec(Partition) *partitionTable;
   short partitionWidth, partitionHeight;
   size_t partitionCount;

};



static void _gridAddActor(GridManager *self, Actor *a, Partition *partition) {
   GridToken *gt = actorGetGridToken(a);
   if (partition) {
      vecPushBack(size_t)(gt->occupyingPartitions, &partition->index);
      vecPushBack(ActorPtr)(partition->actors, &a);
   }
}

static void _gridRemoveActor(GridManager *self, Actor *a, Partition *old) {
   GridToken *gt = actorGetGridToken(a);
   if (gt) {
      vec(size_t) *nodes = gt->occupyingPartitions;

      if (old) {
         vecRemove(size_t)(nodes, &old->index);
         vecRemove(ActorPtr)(old->actors, &a);
      }
   }
}

static void _gridMoveActor(GridManager *self, Actor *a, Partition *old, Partition *new) {

   if (old && new && old != new) {
      _gridRemoveActor(self, a, old);
      _gridAddActor(self, a, new);
   }
}

static Partition *_partitionAt(GridManager *self, size_t index) {
   if (index < self->partitionCount) {
      Partition *p = vecAt(Partition)(self->partitionTable, index);
      if (!p->actors) {
         p->actors = vecCreate(ActorPtr)(NULL);
         p->index = index;
      }
      return p;
   }
   else {
      return NULL;
   }
}

static Partition *_partitionFromXY(GridManager *self, short x, short y) {
   size_t index;
   Partition *p;
   if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
      return NULL;
   }

   index = self->partitionWidth * (y / PARTITION_SIZE) + (x / PARTITION_SIZE);
   p = vecAt(Partition)(self->partitionTable, index);
   if (!p->actors) {
      p->actors = vecCreate(ActorPtr)(NULL);
      p->index = index;
   }

   return p;
}

static void _rebuildPartitionTable(GridManager *self) {
   //clear current partitions for every actor currently loaded
   vecForEach(GridTokenPtr, gtp, self->tokens, {
      GridToken *gt = *gtp;
      vecClear(size_t)(gt->occupyingPartitions);
   });

   //part table
   vecClear(Partition)(self->partitionTable);
   self->partitionWidth = self->width / PARTITION_SIZE + (self->width%PARTITION_SIZE ? 1 : 0);
   self->partitionHeight = self->height / PARTITION_SIZE + (self->height%PARTITION_SIZE ? 1 : 0);
   self->partitionCount = self->partitionHeight * self->partitionWidth;
   vecResize(Partition)(self->partitionTable, self->partitionWidth * self->partitionHeight, &(Partition){NULL});

   //go back voer and reinsert every actor
   vecForEach(GridTokenPtr, gtp, self->tokens, {
      GridToken *gt = *gtp;
      Actor *a = gt->owner;
      Int2 aPos = actorGetGridPosition(a);

      _gridAddActor(self, a, _partitionFromXY(self, aPos.x, aPos.y));
   });
}


static void _createTestGrid(GridManager *self) {
   int i;
   self->width = 21,self->height = 11;
   self->cellCount = self->width * self->height;

   self->map = mapCreate(self->width, self->height);
   
   for (i = 0; i < (int)self->cellCount; ++i) {
      Tile *t = mapTileAt(self->map, i);
      tileSetSchema(t, appRand(appGet(), 1, 7));
   }

   _rebuildPartitionTable(self);

}

void gridTokenMove(GridToken *self, Int2 newPos) {
   Int2 oldPos = actorGetGridPosition(self->owner);
   Partition *old = _partitionFromXY(self->parent, oldPos.x, oldPos.y);
   Partition *new = _partitionFromXY(self->parent, newPos.x, newPos.y);

   _gridMoveActor(self->parent, self->owner, old, new);
}

void gridManagerSetAmbientLight(GridManager *self, byte level) {
   lightGridSetAmbientLight(self->lightGrid, level);
}

int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid) {
   Viewport *vp = self->view->viewport;
   int x, y;
   int vpx = vp->worldPos.x / GRID_CELL_SIZE;
   int vpy = vp->worldPos.y / GRID_CELL_SIZE;
   int count = 0;


   *area = (Recti){
      MAX(0, MIN(self->width - 1, area->left + vpx)),
      MAX(0, MIN(self->height - 1, area->top + vpy)),
      MAX(0, MIN(self->width - 1, area->right + vpx)),
      MAX(0, MIN(self->height - 1, area->bottom + vpy))
   };

   for (y = area->top; y <= area->bottom; ++y) {
      for (x = area->left; x <= area->right; ++x) {
         int worldGridIndex = y * self->width + x;
         Tile *t = mapTileAt(self->map, worldGridIndex);         
         byte occlusionLevel = gridManagerGetSchema(self, tileGetSchema(t))->occlusion;
         if (occlusionLevel > 0) {
            OcclusionCell cell = { 0 };
            cell.level = occlusionLevel;
            cell.x = x - vpx;
            cell.y = y - vpy;
            cell.wx = x;
            cell.wy = y;
            grid[count++] = cell;
         }
      }
   }

   return count;
}

short gridManagerWidth(GridManager *self) {
   return self->width;
}
short gridManagerHeight(GridManager *self) {
   return self->height;
}
size_t gridManagerCellIDFromXY(GridManager *self, int x, int y) {
   size_t out = y * self->width + x;
   if (out >= self->cellCount) {
      return INF;
   }

   return out;
}
size_t gridManagerCellFromTile(GridManager *self, Tile *t) {
   return mapTileIndexFromPointer(self->map, t);
}
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y) {
   if (ID < self->cellCount) {
      *y = ID / self->width;
      *x = ID % self->width;
   }
}
Tile *gridManagerTileAt(GridManager *self, size_t index) {
   if (index < self->cellCount) {
      return mapTileAt(self->map, index);
   }
   return NULL;
}
Tile *gridManagerTileAtXY(GridManager *self, int x, int y) {
   return gridManagerTileAt(self, y*self->width + x);
}

Tile *gridManagerTileAtScreenPos(GridManager *self, int x, int y) {
   Int2 vpPos = screenToWorld(self->view, (Int2) { x, y });
   vpPos.x /= GRID_CELL_SIZE;
   vpPos.y /= GRID_CELL_SIZE;

   return gridManagerTileAtXY(self, vpPos.x, vpPos.y);
}

Map *gridManagerGetMap(GridManager *self) {
   return self->map;
}
void gridManagerLoadMap(GridManager *self, Map *map) {
   size_t i = 0;
   if (self->map && map != self->map) {
      mapDestroy(self->map);
   }
   self->map = map;
   self->width = mapWidth(map);
   self->height = mapHeight(map);
   self->cellCount = self->width * self->height;

   _rebuildPartitionTable(self);

   lightGridLoadMap(self->lightGrid, self->width, self->height);

   //we need to do this on load now for all the lights to be registered
   for (i = 0; i < self->cellCount; ++i) {
      TileSchema *s = gridManagerGetSchema(self, tileGetSchema(mapTileAt(self->map, i)));
      lightGridChangeTileSchema(self->lightGrid, i, s);
   }
}


TileSchema *gridManagerGetSchema(GridManager *self, size_t index) {
   if (index >= vecSize(TileSchema)(self->schemas)) {
      static TileSchema out = { 0 };
      return &out;
   }

   return vecAt(TileSchema)(self->schemas, index);
}

static void _addNewSchema(GridManager *self, DBTileSchema *fromDB) {
   TileSchema newSchema = { 0 };
   
   newSchema.sprite = spriteManagerGetSprite(self->view->spriteManager, stringIntern(c_str(fromDB->sprite)));
   newSchema.occlusion = fromDB->occlusion;
   newSchema.lit = fromDB->lit;
   if (newSchema.lit) {
      newSchema.centerLevel = fromDB->centerLevel;
      newSchema.fadeWidth = fromDB->fadeWidth;
      newSchema.radius = fromDB->radius;
   }

   spriteAttachToGlobalSpeed(newSchema.sprite);
   vecPushBack(TileSchema)(self->schemas, &newSchema);
}

void gridManagerLoadSchemaTable(GridManager *self, const char *set) {
   size_t i = 0;
   vec(DBTileSchema) *schemas = dbTileSchemaSelectByset(self->view->db, set);

   if (schemas) {
      vecClear(TileSchema)(self->schemas);
      vecForEach(DBTileSchema, s, schemas, {
         _addNewSchema(self, s);
      });
      vecDestroy(DBTileSchema)(schemas);

      for (i = 0; i < self->cellCount; ++i) {
         TileSchema *s = gridManagerGetSchema(self, tileGetSchema(mapTileAt(self->map, i)));
         lightGridChangeTileSchema(self->lightGrid, i, s);
      }
   }
}

static void _tileSchemaDestroy(TileSchema *self) {
   spriteDestroy(self->sprite);
}

GridManager *gridManagerCreate(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;

   out->partitionTable = vecCreate(Partition)(&_partitionDestroy);
   out->inViewActors = vecCreate(ActorPtr)(NULL);
   out->schemas = vecCreate(TileSchema)(&_tileSchemaDestroy);

   out->tokens = vecCreate(GridTokenPtr)(&gridTokenPtrDestroy);

   out->lightGrid = lightGridCreate(out);
   //_createTestSchemas(out);
   _createTestGrid(out);


   return out;
}

void gridManagerDestroy(GridManager *self) {
   if (self->map) {
      mapDestroy(self->map);
   }
   lightGridDestroy(self->lightGrid);

   vecDestroy(GridTokenPtr)(self->tokens);
   vecDestroy(Partition)(self->partitionTable);
   vecDestroy(ActorPtr)(self->inViewActors);
   vecDestroy(TileSchema)(self->schemas);
   
   checkedFree(self);
}

LightSource *gridManagerCreateLightSource(GridManager *self) {
   return lightGridCreateLightSource(self->lightGrid);
}

GridToken *gridManagerCreateToken(GridManager *self, Actor *owner) {
   GridToken *out = checkedCalloc(1, sizeof(GridToken));
   out->parent = self;
   out->owner = owner;
   out->occupyingPartitions = vecCreate(size_t)(NULL);
   vecPushBack(GridTokenPtr)(self->tokens, &out);
   return out;
}

void gridTokenDestroy(GridToken *self) {

   //remove from partiitons
   vecForEach(size_t, node, self->occupyingPartitions, {
      Partition *old = _partitionAt(self->parent, *node);
      if (old) {
         vecRemove(ActorPtr)(old->actors, &self->owner);
      }
   });
   
   vecRemove(GridTokenPtr)(self->parent->tokens, &self);
}

void gridManagerUpdate(GridManager *self) {

}

vec(ActorPtr) *gridManagerQueryActors(GridManager *self) {
   Viewport *vp = self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   Recti area = { x, y, x + xcount, y + ycount };

   
   gridManagerQueryActorsRect(self, area, self->inViewActors);
   return self->inViewActors;
}

void gridManagerQueryActorsRect(GridManager *self, Recti area, vec(ActorPtr) *outlist) {
   int x, y;
   int xstart = area.left / PARTITION_SIZE, xend = area.right / PARTITION_SIZE;
   int ystart = area.top / PARTITION_SIZE, yend = area.bottom / PARTITION_SIZE;
   vecClear(ActorPtr)(outlist);

   for (y = ystart; y <= yend; ++y) {
      for (x = xstart; x <= xend; ++x) {
         Partition *p = _partitionAt(self, y * self->partitionWidth + x);
         if (p && p->actors) {
            vecForEach(ActorPtr, e, p->actors, {
               vecPushBack(ActorPtr)(outlist, e);
            });
         }
      }
   }
}

Actor *gridManagerActorFromScreenPosition(GridManager *self, Int2 pos) {
   Int2 worldPos = screenToWorld(self->view, pos);

   gridManagerQueryActors(self);

   vecForEach(ActorPtr, a, self->inViewActors, {
      Int2 aPos = actorGetWorldPosition(*a);
      Recti area = { aPos.x, aPos.y, aPos.x + GRID_CELL_SIZE, aPos.y + GRID_CELL_SIZE };
      if (rectiContains(area, worldPos)) {
         return *a;
      }
   });

   return NULL;
}

void gridManagerChangeTileSchema(GridManager *self, size_t tile, byte schema) {
   Tile *t = mapTileAt(self->map, tile);
   lightGridChangeTileSchema(self->lightGrid, tile, gridManagerGetSchema(self, schema));
   tileSetSchema(t, schema);
}

size_t gridManagerGetSchemaCount(GridManager *self) {
   return vecSize(TileSchema)(self->schemas);
}


static void _renderBlank(Frame *frame, FrameRegion *region, short x, short y, byte color) {

   frameRenderRect(frame, region, x, y, x + GRID_CELL_SIZE, y + GRID_CELL_SIZE, color);
}

void gridManagerRenderSchema(GridManager *self, size_t index, Frame *frame, FrameRegion *region, short x, short y) {
   Viewport *vp = self->view->viewport;
   TileSchema *schema = gridManagerGetSchema(self, index);

   if (self->lightMode) {
      if (schema->occlusion) {
         if (schema->lit) {
            _renderBlank(frame, region, x, y, 13);
         }
         else {
            _renderBlank(frame, region, x, y, 4);
         }
      }
      else if (schema->lit) {
         _renderBlank(frame, region, x, y, 14);
      }
      else {
         _renderBlank(frame, region, x, y, 15);
      }
   }
   else {
      frameRenderSprite(frame, region, x, y, schema->sprite);
   }
}

void gridManagerRender(GridManager *self, Frame *frame) {
   Viewport *vp = self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   int xstart = x, xend = x + xcount;
   int ystart = y, yend = y + ycount;

   xstart = MAX(0, xstart);
   ystart = MAX(0, ystart);
   xend = MIN(self->width, xend);
   yend = MIN(self->height, yend);

   if (!self->tilePalette) {
      self->tilePalette = imageLibraryGetImage(self->view->imageLibrary, stringIntern(IMG_TILE_ATLAS));
   }

   lightGridUpdate(self->lightGrid, x, y);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            if (lightDataGetLevel(lightLevel) > 0) {
               int gridIndex = y * self->width + x;
               size_t schema = tileGetSchema(mapTileAt(self->map, gridIndex));
               FrameRegion *region = &self->view->viewport->region;
               short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
               short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;


               gridManagerRenderSchema(self, schema, frame, region, renderX, renderY);
            }
         }
      }
   }
}

void gridManagerRenderLighting(GridManager *self, Frame *frame) {
   Viewport *vp = self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   int xstart = x, xend = x + xcount;
   int ystart = y, yend = y + ycount;

   xstart = MAX(0, xstart);
   ystart = MAX(0, ystart);
   xend = MIN(self->width, xend);
   yend = MIN(self->height, yend);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
         short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;

         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            lightDataRender(lightLevel, frame, &vp->region, renderX, renderY);
         }
      }
   }

}

int gridDistance(int x0, int y0, int x1, int y1) {
   return abs(x0 - x1) + abs(y0 - y1);
}
void gridManagerToggleLightMode(GridManager *self) {
   self->lightMode = !self->lightMode;
}

void gridManagerRenderGridLineTest(GridManager *self, Frame *frame) {
   Viewport *vp = self->view->viewport;
   Int2 wp = self->view->viewport->worldPos;
   Int2 start = { 0 };
   Int2 end = screenToWorld(self->view, mouseGetPosition(appGetMouse(appGet())));

   int dx = end.x - start.x;
   int dy = end.y - start.y;
   int steps = dx > dy ? abs(dx) : abs(dy);
   float xinc = dx / (float)steps;
   float yinc = dy / (float)steps;
   float fx = start.x, fy = start.y;
   int i = 0;
   size_t currentTile = INF;
   float iSize = 1.0f / GRID_CELL_SIZE;


   for (i = 0; i < steps; ++i) {
      size_t t = 0;
      int ix, iy;
      fx += xinc;
      fy += yinc;

      ix = (int)(fx * iSize);
      iy = (int)(fy * iSize);

      t = gridManagerCellIDFromXY(self, ix, iy);
      if (t != currentTile) {
         int rx = ix * GRID_CELL_SIZE - wp.x;
         int ry = iy * GRID_CELL_SIZE - wp.y;
         currentTile = t;
         frameRenderRect(frame, &vp->region, rx, ry, rx + GRID_CELL_SIZE, ry + GRID_CELL_SIZE, 15);
      }
   }

   frameRenderLine(frame, &vp->region, start.x - wp.x, start.y - wp.y, end.x - wp.x, end.y - wp.y, 0);
}