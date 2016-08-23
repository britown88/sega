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

#define SCHEMA_COUNT 256
#define PARTITION_SIZE 16

#pragma pack(push, 1)

typedef struct {
   vec(ActorPtr) *actors;
   size_t index;
}Partition;

#pragma pack(pop)

static void _partitionDestroy(Partition *p) {
   if (p->actors) {
      vecDestroy(ActorPtr)(p->actors);
   }
}


#define VectorT Partition
#include "segautils/Vector_Create.h"

#define VectorT TileSchema
#include "segautils/Vector_Create.h"

struct GridManager_t {
   WorldView *view;

   //The tile atlas
   ManagedImage *tilePalette;

   //the schema table
   vec(TileSchema) *schemas;

   //the actual grid
   short height, width;
   size_t cellCount;
   LightGrid *lightGrid;
   Map *map;

   //the tile animation clock
   byte tileAnimFrameIndex;
   int tileAnimSecondCount;

   // entities needing drawing this frame (ref to this is returned)
   vec(ActorPtr) *inViewActors;

   // entity partition members
   vec(Partition) *partitionTable;
   short partitionWidth, partitionHeight;
   size_t partitionCount;

};

static Tile *_tileAt(GridManager *self, int x, int y) {
   return mapGetTiles(self->map) + (y * self->width + x);
}


static void _createTestGrid(GridManager *self) {
   int i;
   self->width = 21,self->height = 11;
   self->cellCount = self->width * self->height;

   self->map = mapCreate(self->width, self->height);
   
   for (i = 0; i < (int)self->cellCount; ++i) {
      Tile *grid = mapGetTiles(self->map);
      grid[i] = (Tile) {appRand(appGet(), 1, 7), 0};
      if (grid[i].schema == 6 || grid[i].schema == 5) {
         //self->grid[i].collision = GRID_SOLID;
      }
   }

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
   Recti worldArea = {
      MAX(0, MIN(self->width - 1, area->left + vpx)),
      MAX(0, MIN(self->height - 1, area->top + vpy)),
      MAX(0, MIN(self->width - 1, area->right + vpx)),
      MAX(0, MIN(self->height - 1, area->bottom + vpy))
   };

   for (y = worldArea.top; y <= worldArea.bottom; ++y) {
      for (x = worldArea.left; x <= worldArea.right; ++x) {
         int worldGridIndex = y * self->width + x;
         Tile *mapGrid = mapGetTiles(self->map);
         
         byte occlusionLevel = gridManagerGetSchema(self, mapGrid[worldGridIndex].schema)->occlusion;

         if (occlusionLevel > 0) {
            grid[count++] = (OcclusionCell) {.level = occlusionLevel, .x = x - vpx, .y = y - vpy };
         }
      }
   }
   
   return count;
}

void gridManagerSnapActor(GridManager *self, Actor *a) {
   //GridComponent *gc = entityGet(GridComponent)(e);
   //PositionComponent *pc = entityGet(PositionComponent)(e);

   //if (gc && pc) {
   //   pc->x = gc->x * GRID_CELL_SIZE;
   //   pc->y = gc->y * GRID_CELL_SIZE;
   //}
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
void gridManagerXYFromCellID(GridManager *self, size_t ID, int *x, int *y) {
   if (ID < self->cellCount) {
      *y = ID / self->width;
      *x = ID % self->width;
   }
}
Tile *gridManagerTileAt(GridManager *self, size_t index) {
   if (index < self->cellCount) {
      return mapGetTiles(self->map) + index;
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
   if (self->map && map != self->map) {
      mapDestroy(self->map);
   }
   self->map = map;
   self->width = mapWidth(map);
   self->height = mapHeight(map);
   self->cellCount = self->width * self->height;

   //_rebuildPartitionTable(self);
}
TileSchema *gridManagerGetSchema(GridManager *self, size_t index) {
   size_t count = vecSize(TileSchema)(self->schemas);
   if (index >= count) {
      vecResize(TileSchema)(self->schemas, index + 1, &(TileSchema){0});
   }

   return vecAt(TileSchema)(self->schemas, index);
}

void gridManagerClearSchemas(GridManager *self) {
   vecClear(TileSchema)(self->schemas);
}

GridManager *gridManagerCreate(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;

   out->partitionTable = vecCreate(Partition)(&_partitionDestroy);
   out->inViewActors = vecCreate(ActorPtr)(NULL);
   out->schemas = vecCreate(TileSchema)(NULL);

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

   vecDestroy(Partition)(self->partitionTable);
   vecDestroy(ActorPtr)(self->inViewActors);
   vecDestroy(TileSchema)(self->schemas);
   
   checkedFree(self);
}

void gridManagerUpdate(GridManager *self) {

}

vec(ActorPtr) *gridManagerQueryActors(GridManager *self) {
   Viewport *vp = self->view->viewport;
   //bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   //bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   //byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   //byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   //int x = vp->worldPos.x / GRID_CELL_SIZE;
   //int y = vp->worldPos.y / GRID_CELL_SIZE;

   //Recti area = { x, y, x + xcount, y + ycount };

   //
   //gridManagerQueryEntitiesRect(self, area, self->inViewEntities);
   return self->inViewActors;
}

void gridManagerQueryActorsRect(GridManager *self, Recti area, vec(ActorPtr) *outlist) {
   //int x, y;
   //int xstart = area.left / PARTITION_SIZE, xend = area.right / PARTITION_SIZE;
   //int ystart = area.top / PARTITION_SIZE, yend = area.bottom / PARTITION_SIZE;
   //vecClear(EntityPtr)(outlist);

   //for (y = ystart; y <= yend; ++y) {
   //   for (x = xstart; x <= xend; ++x) {
   //      Partition *p = _partitionAt(self, y * self->partitionWidth + x);
   //      if (p && p->entities) {
   //         vecForEach(EntityPtr, e, p->entities, {
   //            vecPushBack(EntityPtr)(outlist, e);
   //         });
   //      }
   //   }
   //}
}

Actor *gridManagerActorFromScreenPosition(GridManager *self, Int2 pos) {
   Int2 worldPos = screenToWorld(self->view, pos);

   gridManagerQueryActors(self);

   //vecForEach(ActorPtr, a, self->inViewEntities, {
   //   PositionComponent *pc = entityGet(PositionComponent)(*e);
   //   Recti area = { pc->x, pc->y, pc->x + GRID_CELL_SIZE, pc->y + GRID_CELL_SIZE };
   //   if (rectiContains(area, worldPos)) {
   //      return *e;
   //   }
   //});

   return NULL;
}

size_t gridManagerGetSchemaCount(GridManager *self) {
   return vecSize(TileSchema)(self->schemas);
}
short _getImageIndex(GridManager *self, TileSchema *schema) {
   return schema->img[self->tileAnimFrameIndex % schema->imgCount];
}

void gridManagerRenderSchema(GridManager *self, size_t index, Frame *frame, FrameRegion *region, short x, short y) {
   Viewport *vp = self->view->viewport;
   TileSchema *schema = gridManagerGetSchema(self, index);

   if (!schema->imgCount) {
      return;
   }
  
   short img = _getImageIndex(self, schema);
   short imgX = (img % 16) * GRID_CELL_SIZE;
   short imgY = (img / 16) * GRID_CELL_SIZE;

   frameRenderImagePartial(frame, region, x, y, managedImageGetImage(self->tilePalette), imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
}

static void _renderBlank(GridManager *self, Frame *frame, short x, short y) {
   Viewport *vp = self->view->viewport;
   FrameRegion *region = &self->view->viewport->region;
   short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
   short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;
   frameRenderRect(frame, region, renderX, renderY, renderX + GRID_CELL_SIZE, renderY + GRID_CELL_SIZE, 0);
}

void _updateTileAnimationIndex(GridManager *self) {
   int currentSecond = (int)t_u2s(appGetTime(appGet()));
   if (currentSecond != self->tileAnimSecondCount) {
      self->tileAnimSecondCount = currentSecond;
      self->tileAnimFrameIndex = (self->tileAnimFrameIndex + 1) ;
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

   _updateTileAnimationIndex(self);
   lightGridUpdate(self->lightGrid, x, y);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            if (lightLevel->level > 0) {
               int gridIndex = y * self->width + x;
               Tile *mapGrid = mapGetTiles(self->map);
               size_t schema = mapGrid[gridIndex].schema;
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