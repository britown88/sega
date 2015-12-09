#include "MapEditor.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "GridManager.h"

struct MapEditor_t {
   WorldView *view;
   bool enabled;
};

MapEditor *mapEditorCreate(WorldView *view) {
   MapEditor *out = checkedCalloc(1, sizeof(MapEditor));
   out->view = view;
   return out;
}
void mapEditorDestroy(MapEditor *self) {
   checkedFree(self);
}

void mapEditorInitialize(MapEditor *self) {

}
void mapEditorSetEnabled(MapEditor *self, bool enabled) {
   self->enabled = enabled;
}

void mapEditorRender(MapEditor *self, Frame *frame) {
   static int schemaX = 41;
   static int schemaY = 165;
   static int cols = 17;
   static int rows = 2;
   GridManager *gm = self->view->managers->gridManager;
   size_t count = gridManagerGetSchemaCount(gm);
   int x, y, i = 0;

   for (y = 0; y < rows; ++y) {
      short rendery = schemaY + (y * GRID_CELL_SIZE);
      for (x = 0; x < cols; ++x) {
         short renderX = schemaX + (x * GRID_CELL_SIZE);

         gridManagerRenderSchema(gm, i, frame, FrameRegionFULL, renderX, rendery);

         if (++i >= (int)count) {
            return;
         }
      }
   }
}


