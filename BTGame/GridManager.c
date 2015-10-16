#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "WorldView.h"
#include "GridManager.h"

#define SCHEMA_COUNT 256;

typedef struct {
   short image_index;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;
}Tile;

struct GridManager_t {
   Manager m;
   WorldView *view;

   Image *tilePalette;
   TileSchema *schemas;
   short height, width;
   Tile *grid;
};

ImplManagerVTable(GridManager)

static void _createTestSchemas(GridManager *self) {
   int i;
   self->schemas = checkedCalloc(3, sizeof(TileSchema));
   for (i = 0; i < 3; ++i) {
      self->schemas[i] = (TileSchema) { i };
   }   
}

static void _createTestGrid(GridManager *self) {
   int i, count;
   self->width = self->height = 1024;
   count = self->width * self->height;
   self->grid = checkedCalloc(count, sizeof(Tile));
   for (i = 0; i < count; ++i) {
      self->grid[i] = (Tile) {appRand(appGet(), 1, 3), 0};
   }

}

GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);

   out->tilePalette = imageDeserializeOptimized("assets/img/tiles.ega");
   _createTestSchemas(out);
   _createTestGrid(out);

   return out;
}

void _destroy(GridManager *self) {
   if (self->grid) {
      checkedFree(self->grid);
   }

   imageDestroy(self->tilePalette);
   
   checkedFree(self->schemas);
   checkedFree(self);
}
void _onDestroy(GridManager *self, Entity *e) {}
void _onUpdate(GridManager *self, Entity *e) {}

void gridManagerUpdate(GridManager *self) {

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

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         int gridIndex = y * self->width + x;
         short img = self->schemas[self->grid[gridIndex].schema].image_index;
         short imgX = (img % 16) * GRID_CELL_SIZE;
         short imgY = (img / 16) * GRID_CELL_SIZE;

         short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
         short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;
         
         frameRenderImagePartial(frame, &vp->region, renderX, renderY, self->tilePalette,
            imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
      }
   }


}