#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "CoreComponents.h"
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
   vec(EntityPtr) *entities;
   size_t index;
}Partition;

typedef struct {
   vec(size_t) *occupyingPartitions;
}TGridComponent;

#pragma pack(pop)

static void _partitionDestroy(Partition *p) {
   if (p->entities) {
      vecDestroy(EntityPtr)(p->entities);
   }
}

#define TComponentT TGridComponent
#include "Entities\ComponentDeclTransient.h"

#define VectorT Partition
#include "segautils/Vector_Create.h"

#define VectorT TileSchema
#include "segautils/Vector_Create.h"

struct GridManager_t {
   Manager m;
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
   vec(EntityPtr) *inViewEntities;

   // entity partition members
   vec(Partition) *partitionTable;
   short partitionWidth, partitionHeight;
   size_t partitionCount;

};

ImplManagerVTable(GridManager)

static Tile *_tileAt(GridManager *self, int x, int y) {
   return mapGetTiles(self->map) + (y * self->width + x);
}

static void _gridAddEntity(GridManager *self, Entity *e, Partition *partition) {
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (partition) {
      vecPushBack(size_t)(tgc->occupyingPartitions, &partition->index);
      vecPushBack(EntityPtr)(partition->entities, &e);
   }
}

static void _gridRemoveEntity(GridManager *self, Entity *e, Partition *old) {
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (tgc) {
      vec(size_t) *nodes = tgc->occupyingPartitions;

      if (old) {
         vecRemove(size_t)(nodes, &old->index);
         vecRemove(EntityPtr)(old->entities, &e);
      }
   }
}

static void _gridMoveEntity(GridManager *self, Entity *e, Partition *old, Partition *new) {

   if (old && new && old != new) {
      _gridRemoveEntity(self, e, old);
      _gridAddEntity(self, e, new);
   }
}

static Partition *_partitionAt(GridManager *self, size_t index) {
   if (index < self->partitionCount) {
      Partition *p = vecAt(Partition)(self->partitionTable, index);
      if (!p->entities) {
         p->entities = vecCreate(EntityPtr)(NULL);
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
   if (!p->entities) {
      p->entities = vecCreate(EntityPtr)(NULL);
      p->index = index;
   }

   return p;
}

static void _rebuildPartitionTable(GridManager *self) {
   //clear current partitions for every entity currently loaded
   COMPONENT_QUERY(self->view->entitySystem, TGridComponent, tgc, {
      vecClear(size_t)(tgc->occupyingPartitions);
   });

   //part table
   vecClear(Partition)(self->partitionTable);
   self->partitionWidth = self->width / PARTITION_SIZE + (self->width%PARTITION_SIZE ? 1 : 0);
   self->partitionHeight = self->height / PARTITION_SIZE + (self->height%PARTITION_SIZE ? 1 : 0);
   self->partitionCount = self->partitionHeight * self->partitionWidth;
   vecResize(Partition)(self->partitionTable, self->partitionWidth * self->partitionHeight, &(Partition){NULL});

   //go back voer and reinsert every entity
   COMPONENT_QUERY(self->view->entitySystem, TGridComponent, tgc, {
      Entity *e = componentGetParent(tgc, self->view->entitySystem);
      GridComponent *gc = entityGet(GridComponent)(e);
      _gridAddEntity(self, e, _partitionFromXY(self, gc->x, gc->y));
   });
}

static void _createTestSchemas(GridManager *self) {
   int i;

   for (i = 0; i < 8; ++i) {
      *gridManagerGetSchema(self, i) = (TileSchema) { .img = { i }, .imgCount = 1, .occlusion = 0 };
   }   

   gridManagerGetSchema(self, 5)->img[1] = 21;
   gridManagerGetSchema(self, 5)->imgCount = 2;

   gridManagerGetSchema(self, 6)->occlusion = 1;
   gridManagerGetSchema(self, 7)->occlusion = 1;

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

   _rebuildPartitionTable(self);
}

static void _gridComponentUpdate(GridManager *self, Entity *e, GridComponent *oldGC) {
   GridComponent *gc = entityGet(GridComponent)(e);

   Partition *old = _partitionFromXY(self, oldGC->x, oldGC->y);
   Partition *new = _partitionFromXY(self, gc->x, gc->y);

   _gridMoveEntity(self, e, old, new);
}

static void _registerUpdateDelegate(GridManager *self, EntitySystem *system) {
   ComponentUpdate update;

   closureInit(ComponentUpdate)(&update, self, (ComponentUpdateFunc)&_gridComponentUpdate, NULL);
   compRegisterUpdateDelegate(GridComponent)(system, update);
}

static void _removeEntityFromNode(GridManager *self, Entity *e, size_t node) {
   Partition *old = _partitionAt(self, node);
   if (old) {
      vecRemove(EntityPtr)(old->entities, &e);
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

void gridManagerSnapEntity(GridManager *self, Entity *e) {
   GridComponent *gc = entityGet(GridComponent)(e);
   PositionComponent *pc = entityGet(PositionComponent)(e);

   if (gc && pc) {
      pc->x = gc->x * GRID_CELL_SIZE;
      pc->y = gc->y * GRID_CELL_SIZE;
   }
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

   _rebuildPartitionTable(self);
}
TileSchema *gridManagerGetSchema(GridManager *self, short index) {
   size_t count = vecSize(TileSchema)(self->schemas);
   if (index >= (short)count) {
      vecResize(TileSchema)(self->schemas, index + 1, &(TileSchema){0});
   }

   return vecAt(TileSchema)(self->schemas, index);
}

void gridManagerClearSchemas(GridManager *self) {
   vecClear(TileSchema)(self->schemas);
}

GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);
   out->partitionTable = vecCreate(Partition)(&_partitionDestroy);
   out->inViewEntities = vecCreate(EntityPtr)(NULL);
   out->schemas = vecCreate(TileSchema)(NULL);

   out->tilePalette = imageLibraryGetImage(view->imageLibrary, stringIntern("assets/img/tiles.ega"));
   out->lightGrid = lightGridCreate(out);
   _createTestSchemas(out);
   _createTestGrid(out);

   _registerUpdateDelegate(out, view->entitySystem);


   return out;
}

void _destroy(GridManager *self) {
   if (self->map) {
      mapDestroy(self->map);
   }
   lightGridDestroy(self->lightGrid);

   vecDestroy(Partition)(self->partitionTable);
   vecDestroy(EntityPtr)(self->inViewEntities);
   vecDestroy(TileSchema)(self->schemas);
   
   checkedFree(self);
}
void _onDestroy(GridManager *self, Entity *e) {
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   if (tgc) {
      vecForEach(size_t, node, tgc->occupyingPartitions, {
         Partition *old = _partitionAt(self, *node);
         if (old) {
            vecRemove(EntityPtr)(old->entities, &e);
         }
      });
      vecDestroy(size_t)(tgc->occupyingPartitions);
   }
}
void _onUpdate(GridManager *self, Entity *e) {
   TGridComponent *tgc = entityGet(TGridComponent)(e);
   GridComponent *gc = entityGet(GridComponent)(e);

   if (gc) {
      if (!tgc) {
         PositionComponent *pc = entityGet(PositionComponent)(e);
         if (pc) {
            pc->x = gc->x * GRID_CELL_SIZE;
            pc->y = gc->y * GRID_CELL_SIZE;
         }

         //new grid entry
         COMPONENT_ADD(e, TGridComponent, vecCreate(size_t)(NULL));
         _gridAddEntity(self, e, _partitionFromXY(self, gc->x, gc->y));
      }
   }
   else {
      if (tgc) {
         //no longer on grid, remove from occupying nodes
         vecForEach(size_t, node, tgc->occupyingPartitions, {
            _removeEntityFromNode(self, e, *node);
         });
         vecDestroy(size_t)(tgc->occupyingPartitions);
         entityRemove(TGridComponent)(e);
      }
   }
}

void gridManagerUpdate(GridManager *self) {

}

vec(EntityPtr) *gridManagerQueryEntities(GridManager *self) {
   Viewport *vp = self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   Recti area = { x, y, x + xcount, y + ycount };

   
   gridManagerQueryEntitiesRect(self, area, self->inViewEntities);
   return self->inViewEntities;
}

void gridManagerQueryEntitiesRect(GridManager *self, Recti area, vec(EntityPtr) *outlist) {
   int x, y;
   int xstart = area.left / PARTITION_SIZE, xend = area.right / PARTITION_SIZE;
   int ystart = area.top / PARTITION_SIZE, yend = area.bottom / PARTITION_SIZE;
   vecClear(EntityPtr)(outlist);

   for (y = ystart; y <= yend; ++y) {
      for (x = xstart; x <= xend; ++x) {
         Partition *p = _partitionAt(self, y * self->partitionWidth + x);
         if (p && p->entities) {
            vecForEach(EntityPtr, e, p->entities, {
               vecPushBack(EntityPtr)(outlist, e);
            });
         }
      }
   }
}

Entity *gridMangerEntityFromScreenPosition(GridManager *self, Int2 pos) {
   Int2 worldPos = screenToWorld(self->view, pos);

   gridManagerQueryEntities(self);

   vecForEach(EntityPtr, e, self->inViewEntities, {
      PositionComponent *pc = entityGet(PositionComponent)(*e);
      Recti area = { pc->x, pc->y, pc->x + GRID_CELL_SIZE, pc->y + GRID_CELL_SIZE };
      if (rectiContains(area, worldPos)) {
         return *e;
      }
   });

   return NULL;
}

static void _renderTile(GridManager *self, Frame *frame, short x, short y, short imgX, short imgY) {
   Viewport *vp = self->view->viewport;
   FrameRegion *region = &self->view->viewport->region;
   short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
   short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;
   frameRenderImagePartial(frame, region, renderX, renderY, managedImageGetImage(self->tilePalette), imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
   //frameRenderRect(frame, vp, x, y, x + GRID_CELL_SIZE, y + GRID_CELL_SIZE, 15);
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
short _getImageIndex(GridManager *self, TileSchema *schema) {
   return schema->img[self->tileAnimFrameIndex % schema->imgCount];
}

void _hideEntitySquare(GridManager *self, Frame *frame, Entity *e, int xstart, int xend, int ystart, int yend) {
   GridComponent *gc = entityGet(GridComponent)(e);
   //size_t lastindex = gridMovementManagerGetEntityLastPosition(self->view->managers->gridMovementManager, e);
   //int lastX = 0, lastY = 0;

   if (gc->x >= xstart && gc->x < xend && gc->y >= ystart && gc->y < yend) {
      _renderBlank(self, frame, gc->x, gc->y);
   }   

   //if (lastindex < INF) {
   //   gridManagerXYFromCellID(self, lastindex, &lastX, &lastY);
   //   if (lastX >= xstart && lastX < xend && lastY >= ystart && lastY < yend) {
   //      _renderBlank(self, frame, lastX, lastY);
   //   }
   //}
}

void _hideEntitySquares(GridManager *self, Frame *frame, int xstart, int xend, int ystart, int yend) {
   gridManagerQueryEntities(self);

   vecForEach(EntityPtr, e, self->inViewEntities, {
      _hideEntitySquare(self, frame, *e, xstart, xend, ystart, yend);
   });
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

   _updateTileAnimationIndex(self);
   lightGridUpdate(self->lightGrid, self->view->entitySystem, x, y);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         int gridIndex = y * self->width + x;
         Tile *mapGrid = mapGetTiles(self->map);
         
         short img = _getImageIndex(self, gridManagerGetSchema(self, mapGrid[gridIndex].schema));
         short imgX = (img % 16) * GRID_CELL_SIZE;
         short imgY = (img / 16) * GRID_CELL_SIZE;

         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            if (lightLevel->level > 0) {
               _renderTile(self, frame, x, y, imgX, imgY);
               //lightDataRender(lightLevel, frame, &vp->region, renderX, renderY);
            }
         }
      }
   }

  // _hideEntitySquares(self, frame, xstart, xend, ystart, yend);
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