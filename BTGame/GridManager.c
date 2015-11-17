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
#include "segautils/IntrusiveHeap.h"

#define SCHEMA_COUNT 256
#define PARTITION_SIZE 16

#define DK_SEARCH_RADIUS 32
#define DK_NEIGHBOR_COUNT 4

#pragma pack(push, 1)
typedef struct {
   short img[3];
   byte imgCount;
   byte occlusion;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;//use "solid" flags
}Tile;

typedef struct {
   vec(EntityPtr) *entities;
   size_t index;
}Partition;

typedef struct {
   vec(size_t) *occupyingPartitions;
}TGridComponent;

typedef struct GridNode_t GridNode;

//used for dijkstra's solves
struct GridNode_t{
   GridNodePublic data;
   QueueNode node;
   GridNode *parent;//the observing node
   GridNode *neighbors[DK_NEIGHBOR_COUNT];
   byte visited;//for node generation
   float score;
};
#pragma pack(pop)

float gridNodeGetScore(GridNodePublic *self) {
   return ((GridNode*)self)->score;
}

static GridNode *_nodeCompareFunc(GridNode *n1, GridNode *n2) {
   return n1->score < n2->score ? n1 : n2;
}

static void _partitionDestroy(Partition *p) {
   if (p->entities) {
      vecDestroy(EntityPtr)(p->entities);
   }
}

#define ClosureTPart CLOSURE_NAME(GridProcessNeighbor)
#include "segautils\Closure_Impl.h"

#define ClosureTPart CLOSURE_NAME(GridProcessCurrent)
#include "segautils\Closure_Impl.h"

#define VectorTPart GridSolutionNode
#include "segautils\Vector_Impl.h"

#define TComponentT TGridComponent
#include "Entities\ComponentDeclTransient.h"

#define VectorT Partition
#include "segautils/Vector_Create.h"

#define VectorT GridNode
#include "segautils/Vector_Create.h"

#pragma region Dijkstra's'
typedef struct {
   Dijkstras inner;
   GridProcessCurrent cFunc;
   GridProcessNeighbor nFunc;
   GridNode *solutionNode;
}GridSolver;

static size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList);
static int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node);
static int _solverProcessCurrent(GridSolver *self, GridNode *node);
static void _solverDestroy(GridSolver *self);

static DijkstrasVTable *_getSolverVTable() {
   DijkstrasVTable *out = NULL;
   if (!out) {
      out = calloc(1, sizeof(DijkstrasVTable));
      out->getNeighbors = (size_t(*)(Dijkstras*, QueueElem, QueueElem**))&_solverGetNeighbors;
      out->processNeighbor = (int(*)(Dijkstras*, QueueElem, QueueElem))&_solverProcessNeighbor;
      out->processCurrent = (int(*)(Dijkstras*, QueueElem))&_solverProcessCurrent;
      out->destroy = (void(*)(Dijkstras*))&_solverDestroy;
   }
   return out;
}

size_t _solverGetNeighbors(GridSolver *self, GridNode *node, GridNode ***outList) {
   *outList = node->neighbors;
   return DK_NEIGHBOR_COUNT;
}
int _solverProcessNeighbor(GridSolver *self, GridNode *current, GridNode *node) {
   if (node && !node->visited) {
      float newScore = closureCall(&self->nFunc, &current->data, &node->data);
      if (newScore < node->score) {
         node->parent = current;
         node->score = newScore;
         return true;
      }
   }
   return false;
}
int _solverProcessCurrent(GridSolver *self, GridNode *node) {
   node->visited = true;
   GridNode *solution = (GridNode*)closureCall(&self->cFunc, &node->data);
   if (solution) {
      self->solutionNode = solution;
      return true;
   }

   return false;
}
void _solverDestroy(GridSolver *self) {
   priorityQueueDestroy(self->inner.queue);
   checkedFree(self);
}

#pragma endregion

struct GridManager_t {
   Manager m;
   WorldView *view;

   //The tile atlas
   ManagedImage *tilePalette;

   //the schema table
   TileSchema *schemas;

   //the actual grid
   short height, width;
   size_t cellCount;
   Tile *grid;
   LightGrid *lightGrid;

   //the tile animation clock
   byte tileAnimFrameIndex;
   int tileAnimSecondCount;

   // entities needing drawing this frame (ref to this is returned)
   vec(EntityPtr) *inViewEntities;

   // entity partition members
   vec(Partition) *partitionTable;
   short partitionWidth, partitionHeight;
   size_t partitionCount;

   //dyjkstras members
   vec(GridNode) *solvingTable;

   //precompute out a few things here for ease of grabbing
   Recti solutionSearchArea;
   size_t startCell; //relative to the search area
   int solveWidth, solveHeight;
   size_t solveCount;

   PriorityQueue *solveQueue;
   GridSolver *solver;
   vec(GridSolutionNode) *solutionMap;
};

ImplManagerVTable(GridManager)

//we need to build our table to encompass our search area
static void _clearSolutionTable(GridManager *self, size_t startCell) {
   int x = 0, y = 0;
   int ix, iy;
   size_t i = 0;
   Recti *r = &self->solutionSearchArea;

   gridManagerXYFromCellID(self, startCell, &x, &y);

   r->left = MAX(0, x - DK_SEARCH_RADIUS);
   r->top = MAX(0, y - DK_SEARCH_RADIUS);
   r->right = MIN(self->width, x + DK_SEARCH_RADIUS);
   r->bottom = MIN(self->height, y + DK_SEARCH_RADIUS);

   self->solveWidth = rectiWidth(r);
   self->solveHeight = rectiHeight(r);
   self->solveCount = self->solveWidth * self->solveHeight;

   vecClear(GridNode)(self->solvingTable);
   vecResize(GridNode)(self->solvingTable, self->solveCount, &(GridNode){0});

   for (iy = r->top; iy < r->bottom; ++iy) {
      for (ix = r->left; ix < r->right; ++ix) {
         GridNode *node = vecAt(GridNode)(self->solvingTable, i++);
         size_t ID = gridManagerCellIDFromXY(self, ix, iy);

         node->data.ID = ID;
         node->data.collision = self->grid[ID].collision;
         node->score = INFF;
         node->visited = false;
         node->parent = NULL;
         queueNodeClear(&node->node);
      }
   }

   //here's some ostentatious bullshit, coffee give me strength
   //set neighbors automagically based on collision
   i = 0;
   vecForEach(GridNode, node, self->solvingTable, {
      int x = i%self->solveWidth;

      if (i >= self->solveWidth) {// y > 0
         GridNode *above = self->solvingTable->data + (i - self->solveWidth);
         if (!(node->data.collision&GRID_SOLID_TOP) && !(above->data.collision&GRID_SOLID_BOTTOM)) {
            node->neighbors[0] = above;
         }
      }

      if (x > 0) {// x > 0
         GridNode *right = self->solvingTable->data + i + 1;
         if (!(node->data.collision&GRID_SOLID_RIGHT) && !(right->data.collision&GRID_SOLID_LEFT)) {
            node->neighbors[1] = right;
         }
      }

      if (i < self->solveCount - self->solveWidth) {// y < height - 1
         GridNode *below = self->solvingTable->data + (i + self->solveWidth);
         if (!(node->data.collision&GRID_SOLID_BOTTOM) && !(below->data.collision&GRID_SOLID_TOP)) {
            node->neighbors[2] = below;
         }
      }

      if (x < self->solveWidth - 1) {// x < width - 1
         GridNode *left = self->solvingTable->data + i - 1;
         if (!(node->data.collision&GRID_SOLID_LEFT) && !(left->data.collision&GRID_SOLID_RIGHT)) {
            node->neighbors[3] = left;
         }
      }
      
      ++i;
   });
}

static size_t _gridIDRelativeToSearchArea(GridManager *self, size_t cell) {
   int x = 0, y = 0;
   gridManagerXYFromCellID(self, cell, &x, &y);
   Recti *r = &self->solutionSearchArea;

   if (x < r->left || x >= r->right || y < r->top || y >= r->bottom) {
      return INF;
   }

   return ((y - r->top) * self->solveWidth) + (x - r->left);
}

GridSolution gridManagerSolve(GridManager *self, size_t startCell, GridProcessCurrent cFunc, GridProcessNeighbor nFunc) {
   GridSolution solution = { INFF, INF, NULL };

   if (startCell < self->cellCount) {
      GridNode *result = NULL;

      _clearSolutionTable(self, startCell);

      //determine the index of the startcell in the newly-made table
      self->startCell = _gridIDRelativeToSearchArea(self, startCell);

      //set the starting cell to 
      vecAt(GridNode)(self->solvingTable, self->startCell)->score = 0.0f;

      //clear the queue
      priorityQueueClear(self->solveQueue);

      vecForEach(GridNode, node, self->solvingTable, {
         priorityQueuePush(self->solveQueue, node);
      });

      self->solver->cFunc = cFunc;
      self->solver->nFunc = nFunc;

      dijkstrasRun((Dijkstras*)self->solver);

      result = self->solver->solutionNode;

      if (result) {
         solution.solutionCell = result->data.ID;
         solution.totalCost = result->score;

         vecClear(GridSolutionNode)(self->solutionMap);

         while (result && result->parent) {
            GridSolutionNode resoluteNode = { .node = result->data.ID };
            vecPushBack(GridSolutionNode)(self->solutionMap, &resoluteNode);
            result = result->parent;
         }

         vecReverse(GridSolutionNode)(self->solutionMap);
         solution.path = self->solutionMap;
      }
   }

   return solution;
}

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
      if (self->grid[i].schema == 6) {
         self->grid[i].collision = GRID_SOLID_TOP | GRID_SOLID_BOTTOM | GRID_SOLID_LEFT | GRID_SOLID_RIGHT;
      }
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
      *x = ID % self->height;
   }
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

   //solving
   out->solutionMap = vecCreate(GridSolutionNode)(NULL);
   out->solvingTable = vecCreate(GridNode)(NULL);

   out->solveQueue = priorityQueueCreate(offsetof(GridNode, node), (PQCompareFunc)&_nodeCompareFunc);

   out->solver = checkedCalloc(1, sizeof(GridSolver));
   out->solver->inner.vTable = _getSolverVTable();
   out->solver->inner.queue = out->solveQueue;

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
   vecDestroy(GridNode)(self->solvingTable);
   vecDestroy(GridSolutionNode)(self->solutionMap);
   dijkstrasDestroy((Dijkstras*)self->solver);
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