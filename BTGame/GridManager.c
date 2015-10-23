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

#define SCHEMA_COUNT 256;

#pragma pack(push, 1)
typedef struct {
   short image_index;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;
   byte occlusion;
}Tile;
#pragma pack(pop)

struct GridManager_t {
   Manager m;
   WorldView *view;

   ManagedImage *tilePalette;
   TileSchema *schemas;
   short height, width;
   Tile *grid;
   LightGrid *lightGrid;
};

ImplManagerVTable(GridManager)



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
         int lightGridIndex = (y - vpy) * rectiWidth(&worldArea) + (x - vpx);
         grid[lightGridIndex].level = self->grid[worldGridIndex].occlusion;
         if (grid[lightGridIndex].level) {
            ++count;
         }
      }
   }
   
   return count;
}

GridManager *createGridManager(WorldView *view) {
   GridManager *out = checkedCalloc(1, sizeof(GridManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(GridManager);

   out->tilePalette = imageLibraryGetImage(view->imageLibrary, stringIntern("assets/img/tiles.ega"));
   out->lightGrid = lightGridCreate(out);
   _createTestSchemas(out);
   _createTestGrid(out);

   return out;
}

void _destroy(GridManager *self) {
   if (self->grid) {
      checkedFree(self->grid);
   }
   lightGridDestroy(self->lightGrid);

   checkedFree(self->schemas);
   checkedFree(self);
}
void _onDestroy(GridManager *self, Entity *e) {}
void _onUpdate(GridManager *self, Entity *e) {
   PositionComponent *pc = entityGet(PositionComponent)(e);
   OcclusionComponent *oc = entityGet(OcclusionComponent)(e);

   if (pc && oc) {
      int x = pc->x / GRID_CELL_SIZE;
      int y = pc->y / GRID_CELL_SIZE;
      int gridIndex;

      if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
         return;
      }

      gridIndex = y * self->width + x;
      self->grid[gridIndex].occlusion = 1;
   }
}

void gridManagerUpdate(GridManager *self) {

}



static void _renderTile(GridManager *self, Frame *frame, short x, short y, short imgX, short imgY) {
   FrameRegion *vp = &self->view->viewport.region;
   frameRenderImagePartial(frame, vp, x, y, managedImageGetImage(self->tilePalette), imgX, imgY, GRID_CELL_SIZE, GRID_CELL_SIZE);
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

   lightGridUpdate(self->lightGrid, self->view->entitySystem, x, y);

   for (y = ystart; y < yend; ++y) {
      for (x = xstart; x < xend; ++x) {
         int gridIndex = y * self->width + x;
         short img = self->schemas[self->grid[gridIndex].schema].image_index;
         short imgX = (img % 16) * GRID_CELL_SIZE;
         short imgY = (img / 16) * GRID_CELL_SIZE;

         short renderX = (x * GRID_CELL_SIZE) - vp->worldPos.x;
         short renderY = (y * GRID_CELL_SIZE) - vp->worldPos.y;

         LightData *lightLevel = lightGridAt(self->lightGrid, x - xstart, y - ystart);
         if (lightLevel) {
            if (lightLevel->level > 0) {
               _renderTile(self, frame, renderX, renderY, imgX, imgY);
               lightDataRender(lightLevel, frame, &vp->region, renderX, renderY);
            }
         }
      }
   }


}