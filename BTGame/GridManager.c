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
   short img[3];
   byte imgCount;
   byte occlusion;
}TileSchema;

typedef struct {
   byte schema;
   byte collision;
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
   byte tileAnimFrameIndex;
   int tileAnimSecondCount;
};

ImplManagerVTable(GridManager)

static Tile *_tileAt(GridManager *self, int x, int y) {
   return self->grid + (y * self->width + x);
}

static void _createTestSchemas(GridManager *self) {
   int i;
   self->schemas = checkedCalloc(8, sizeof(TileSchema));
   for (i = 0; i < 8; ++i) {
      self->schemas[i] = (TileSchema) { .img = { i }, .imgCount = 1, .occlusion = 0 };
   }   

   self->schemas[5].img[1] = 21;
   self->schemas[5].imgCount = 2;

   self->schemas[7].occlusion = 1;
}

static void _createTestGrid(GridManager *self) {
   int i, count;
   self->width = self->height = 1024;
   count = self->width * self->height;
   self->grid = checkedCalloc(count, sizeof(Tile));
   for (i = 0; i < count; ++i) {
      self->grid[i] = (Tile) {appRand(appGet(), 1, 7), 0};
   }
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
void _onUpdate(GridManager *self, Entity *e) {}

void gridManagerUpdate(GridManager *self) {

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
               lightDataRender(lightLevel, frame, &vp->region, renderX, renderY);
            }
         }
      }
   }


}