#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "GridManager.h"
#include "ImageLibrary.h"

#include <math.h>

#define SCHEMA_COUNT 256;

#define LIGHT_GRID_WIDTH (GRID_WIDTH + 1)
#define LIGHT_GRID_HEIGHT (GRID_HEIGHT + 1)
#define LIGHT_GRID_CELL_COUNT (LIGHT_GRID_WIDTH * LIGHT_GRID_HEIGHT)
#define LIGHT_LEVEL_COUNT 7

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

typedef struct {
   short image_index;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;
}Tile;

typedef struct {
   byte level;
}LightData;

struct GridManager_t {
   Manager m;
   WorldView *view;

   ManagedImage *tilePalette;
   TileSchema *schemas;
   short height, width;
   Tile *grid;
   LightData lightGrid[LIGHT_GRID_CELL_COUNT];
};

ImplManagerVTable(GridManager)

static LightData *_lightLevelAt(GridManager *self, byte x, byte y) {
   return self->lightGrid + (y * LIGHT_GRID_WIDTH + x);
}

static void _createTestSchemas(GridManager *self) {
   int i;
   self->schemas = checkedCalloc(5, sizeof(TileSchema));
   for (i = 0; i < 5; ++i) {
      self->schemas[i] = (TileSchema) { i };
   }   
}

static void _createTestGrid(GridManager *self) {
   int i, count;
   self->width = self->height = 1024;
   count = self->width * self->height;
   self->grid = checkedCalloc(count, sizeof(Tile));
   for (i = 0; i < count; ++i) {
      self->grid[i] = (Tile) {appRand(appGet(), 1, 5), 0};
   }
}

static void _renderLight(GridManager *self, byte ox, byte oy, byte radius) {
   int x, y;
   for (y = -radius; y <= radius; ++y) {
      for (x = -radius; x <= radius; ++x) {
         if (ox + x >= 0 && ox + x < LIGHT_GRID_WIDTH &&
            oy + y >= 0 && oy + y < LIGHT_GRID_HEIGHT) {
            if (x*x + y*y <= radius * radius) {
               int dist = sqrtf((x) * (x) + (y) * (y));

               _lightLevelAt(self, ox + x, oy + y)->level = LIGHT_LEVEL_COUNT - dist;
            }
         }
      }
   }
}

static void _createTestLight(GridManager *self) {
   //int i, j;
   ////for (i = 0; i < LIGHT_GRID_CELL_COUNT; ++i) {
   ////   self->lightGrid[i].level = appRand(appGet(), 0, LIGHT_LEVEL_COUNT);
   ////}

   //for (i = 0; i < LIGHT_GRID_WIDTH; ++i) {

   //   for (j = 0; j < LIGHT_GRID_HEIGHT; ++j) {
   //      _lightLevelAt(self, i, j)->level = j;
   //   }
   //   
   //   
   //}

   _renderLight(self, 10, 5, 5);
}



GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);

   out->tilePalette = imageLibraryGetImage(view->imageLibrary, stringIntern("assets/img/tiles.ega"));
   _createTestSchemas(out);
   _createTestGrid(out);
   _createTestLight(out);

   return out;
}

void _destroy(GridManager *self) {
   if (self->grid) {
      checkedFree(self->grid);
   }
   
   checkedFree(self->schemas);
   checkedFree(self);
}
void _onDestroy(GridManager *self, Entity *e) {}
void _onUpdate(GridManager *self, Entity *e) {}

void gridManagerUpdate(GridManager *self) {

}

static void _renderAllLights(GridManager *self, byte vpx, byte vpy) {
   Viewport *vp = &self->view->viewport;
   memset(self->lightGrid, 0, sizeof(self->lightGrid));

   COMPONENT_QUERY(self->view->entitySystem, LightComponent, lc, {
      Entity *e = componentGetParent(lc, self->view->entitySystem);
      PositionComponent *pc = entityGet(PositionComponent)(e);
      _renderLight(self, (pc->x / GRID_CELL_SIZE) - vpx, (pc->y / GRID_CELL_SIZE) - vpy, lc->radius);
   });
}

static void _renderTile(GridManager *self, Frame *frame, LightData *light, short x, short y, short imgX, short imgY) {
   int shadex, shadey, pixel, darkInterval;
   FrameRegion *vp = &self->view->viewport.region;
   if (light->level == 0) {
      return;
   }  

   frameRenderImagePartial(frame, vp, x, y, managedImageGetImage(self->tilePalette), imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
   //frameRenderRect(frame, vp, x, y, x + GRID_CELL_SIZE, y + GRID_CELL_SIZE, 15);

   if (light->level >= LIGHT_LEVEL_COUNT - 1) {
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

void gridManagerRender(GridManager *self, Frame *frame) {
   Viewport *vp = &self->view->viewport;
   bool xaligned = !(vp->worldPos.x % GRID_CELL_SIZE);
   bool yaligned = !(vp->worldPos.y % GRID_CELL_SIZE);

   byte xcount = GRID_WIDTH + (xaligned ? 0 : 1);
   byte ycount = GRID_HEIGHT + (yaligned ? 0 : 1);

   byte x = vp->worldPos.x / GRID_CELL_SIZE;
   byte y = vp->worldPos.y / GRID_CELL_SIZE;

   byte xstart = x, xend = x + xcount;
   byte ystart = y, yend = y + ycount;

   _renderAllLights(self, x, y);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         int gridIndex = y * self->width + x;
         short img = self->schemas[self->grid[gridIndex].schema].image_index;
         short imgX = (img % 16) * GRID_CELL_SIZE;
         short imgY = (img / 16) * GRID_CELL_SIZE;

         short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
         short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;

         LightData *lightLevel = _lightLevelAt(self, x - xstart, y - ystart);
         _renderTile(self, frame, lightLevel, renderX, renderY, imgX, imgY);
      }
   }


}