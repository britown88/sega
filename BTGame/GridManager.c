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

#define SCHEMA_COUNT 256
#define PARTITION_SIZE 16

#pragma pack(push, 1)
typedef struct {
   short img[3];
   byte imgCount;
   byte occlusion;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;
}Tile;

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

struct GridManager_t {
   Manager m;
   WorldView *view;

   ManagedImage *tilePalette;
   TileSchema *schemas;
   short height, width;
   size_t cellCount;
   Tile *grid;
   LightGrid *lightGrid;

   byte tileAnimFrameIndex;
   int tileAnimSecondCount;

   vec(EntityPtr) *inViewEntities;

   vec(Partition) *partitionTable;
   short partitionWidth, partitionHeight;
   size_t partitionCount;
};

ImplManagerVTable(GridManager)

static Tile *_tileAt(GridManager *self, int x, int y) {
   return self->grid + (y * self->width + x);
}

static void _createTestSchemas(GridManager *self) {
   int i;
   self->schemas = checkedCalloc(11, sizeof(TileSchema));
   for (i = 0; i < 8; ++i) {
      self->schemas[i] = (TileSchema) { .img = { i }, .imgCount = 1, .occlusion = 0 };
   }   

   self->schemas[5].img[1] = 21;
   self->schemas[5].imgCount = 2;

   self->schemas[7].occlusion = 1;

   self->schemas[8] = (TileSchema) { .img = { 34 }, .imgCount = 1, .occlusion = 0 };
   self->schemas[9] = (TileSchema) { .img = { 35 }, .imgCount = 1, .occlusion = 0 };
   self->schemas[10] = (TileSchema) { .img = { 36 }, .imgCount = 1, .occlusion = 0 };


}

static void _createTestGrid(GridManager *self) {
   int i, count;
   self->width = self->height = 1024;
   self->cellCount = self->width * self->height;

   count = self->width * self->height;
   self->grid = checkedCalloc(count, sizeof(Tile));
   
   for (i = 0; i < count; ++i) {
      self->grid[i] = (Tile) {appRand(appGet(), 1, 7), 0};
   }

   self->grid[0] = (Tile) { 8, 0 };
   self->grid[1] = (Tile) { 9, 0 };
   self->grid[2] = (Tile) { 10, 0 };

   //part table
   vecClear(Partition)(self->partitionTable);
   self->partitionWidth = self->width / PARTITION_SIZE + (self->width%PARTITION_SIZE ? 1 : 0);
   self->partitionHeight = self->height / PARTITION_SIZE + (self->height%PARTITION_SIZE ? 1 : 0);
   self->partitionCount = self->partitionHeight * self->partitionWidth;
   vecResize(Partition)(self->partitionTable, self->partitionWidth * self->partitionHeight, &(Partition){NULL});
}

static Partition *_partitionAt(GridManager *self, size_t index) {
   if (index < self->partitionCount) {
      return vecAt(Partition)(self->partitionTable, index);
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

void gridManagerSetTileSchema(GridManager *self, int x, int y, byte schema) {
   if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
      return;
   }

   _tileAt(self, x, y)->schema = schema;
}

int gridManagerQueryOcclusion(GridManager *self, Recti *area, OcclusionCell *grid) {
   Viewport *vp = &self->view->viewport;
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
         byte occlusionLevel = self->schemas[self->grid[worldGridIndex].schema].occlusion;

         if (occlusionLevel > 0) {
            grid[count++] = (OcclusionCell) {.level = occlusionLevel, .x = x - vpx, .y = y - vpy };
         }
      }
   }
   
   return count;
}

GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);
   out->partitionTable = vecCreate(Partition)(&_partitionDestroy);
   out->inViewEntities = vecCreate(EntityPtr)(NULL);

   out->tilePalette = imageLibraryGetImage(view->imageLibrary, stringIntern("assets/img/tiles.ega"));
   out->lightGrid = lightGridCreate(out);
   _createTestSchemas(out);
   _createTestGrid(out);

   _registerUpdateDelegate(out, view->entitySystem);

   return out;
}

void _destroy(GridManager *self) {
   if (self->grid) {
      checkedFree(self->grid);      
   }
   lightGridDestroy(self->lightGrid);

   vecDestroy(Partition)(self->partitionTable);
   vecDestroy(EntityPtr)(self->inViewEntities);
   checkedFree(self->schemas);
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
   Viewport *vp = &self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   int xstart = x / PARTITION_SIZE, xend = (x + xcount) / PARTITION_SIZE;
   int ystart = y / PARTITION_SIZE, yend = (y + ycount) / PARTITION_SIZE;

   vecClear(EntityPtr)(self->inViewEntities);

   for (y = ystart; y <= yend; ++y) {
      for (x = xstart; x <= xend; ++x) {
         Partition *p = _partitionAt(self, y * self->partitionWidth + x);
         if (p && p->entities) {
            vecForEach(EntityPtr, e, p->entities, {
               vecPushBack(EntityPtr)(self->inViewEntities, e);
            });
         }
      }
   }

   return self->inViewEntities;
}

static void _renderTile(GridManager *self, Frame *frame, short x, short y, short imgX, short imgY) {
   FrameRegion *vp = &self->view->viewport.region;
   frameRenderImagePartial(frame, vp, x, y, managedImageGetImage(self->tilePalette), imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
   //frameRenderRect(frame, vp, x, y, x + GRID_CELL_SIZE, y + GRID_CELL_SIZE, 15);
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

void gridManagerRender(GridManager *self, Frame *frame) {
   Viewport *vp = &self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   int xstart = x, xend = x + xcount;
   int ystart = y, yend = y + ycount;
   
   _updateTileAnimationIndex(self);
   lightGridUpdate(self->lightGrid, self->view->entitySystem, x, y);   

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         int gridIndex = y * self->width + x;
         short img = _getImageIndex(self, self->schemas + self->grid[gridIndex].schema);
         short imgX = (img % 16) * GRID_CELL_SIZE;
         short imgY = (img / 16) * GRID_CELL_SIZE;

         short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
         short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;

         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            if (lightLevel->level > 0) {
               _renderTile(self, frame, renderX, renderY, imgX, imgY);
               //lightDataRender(lightLevel, frame, &vp->region, renderX, renderY);
            }
         }
      }
   }


}

void gridManagerRenderLighting(GridManager *self, Frame *frame) {
   Viewport *vp = &self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   int x = vp->worldPos.x / GRID_CELL_SIZE;
   int y = vp->worldPos.y / GRID_CELL_SIZE;

   int xstart = x, xend = x + xcount;
   int ystart = y, yend = y + ycount;

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