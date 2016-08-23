#include "MapEditor.h"
#include "Managers.h"
#include "segashared/CheckedMemory.h"
#include "GridManager.h"
#include "RichText.h"
#include "TextArea.h"

#include <stdio.h>

#define STATS_LEFT 2
#define STATS_TOP 2

#define SCHEMA_LEFT 41
#define SCHEMA_TOP 165
#define SCHEMA_COLUMNS 17
#define SCHEMA_ROWS 2

struct MapEditor_t {
   WorldView *view;

   byte schemaIndex;
   byte schemaRowIndex;

   Int2 mouseGridPos;

   TextArea *xyDisplay;
};


MapEditor *mapEditorCreate(WorldView *view) {
   MapEditor *out = checkedCalloc(1, sizeof(MapEditor));
   out->view = view;

   out->schemaIndex = out->schemaRowIndex = 0;
   out->xyDisplay = textAreaCreate(STATS_LEFT, STATS_TOP, EGA_RES_WIDTH, 2);

   return out;
}
void mapEditorDestroy(MapEditor *self) {

   textAreaDestroy(self->xyDisplay);
   checkedFree(self);
}

void mapEditorReset(MapEditor *self) {
   self->schemaIndex = self->schemaRowIndex = 0;
}

bool mapEditorPointInSchemaWindow(MapEditor *self, Int2 p) {
   return 
      p.x >= SCHEMA_LEFT && p.x < (SCHEMA_LEFT + GRID_CELL_SIZE * SCHEMA_COLUMNS) && 
      p.y >= SCHEMA_TOP && p.y < (SCHEMA_TOP + GRID_CELL_SIZE * SCHEMA_ROWS);
}

void mapEditorSelectSchema(MapEditor *self, Int2 mousePos) {
   short schemaX = (mousePos.x - SCHEMA_LEFT) / GRID_CELL_SIZE;
   short schemaY = (mousePos.y - SCHEMA_TOP) / GRID_CELL_SIZE;

   self->schemaIndex =
      (self->schemaRowIndex * SCHEMA_COLUMNS) +
      (schemaY * SCHEMA_COLUMNS) +
      schemaX;
}
void mapEditorScrollSchemas(MapEditor *self, int deltaY) {
   GridManager *gm = self->view->gridManager;
   int sCount = (int)gridManagerGetSchemaCount(gm);
   int sRowCount = (sCount / SCHEMA_COLUMNS) + (sCount % SCHEMA_COLUMNS ? 1 : 0);
   int maxRow = MAX(sRowCount - SCHEMA_ROWS, 0);

   if (deltaY < 0) {
      self->schemaRowIndex = MIN(self->schemaRowIndex + 1, maxRow);
   }
   else if (deltaY > 0) {
      self->schemaRowIndex = MAX(self->schemaRowIndex - 1, 0);
   }
}

byte mapEditorGetSelectedSchema(MapEditor *self) {
   return self->schemaIndex;
}

void mapEditorSetSelectedSchema(MapEditor *self, byte schema) {
   GridManager *gm = self->view->gridManager;
   int sCount = (int)gridManagerGetSchemaCount(gm);
   int sRowCount = (sCount / SCHEMA_COLUMNS) + (sCount % SCHEMA_COLUMNS ? 1 : 0);
   int schemaRow = schema / SCHEMA_COLUMNS;

   if (schema < 0 || schema >= sCount) {
      return;
   }

   self->schemaIndex = schema;
   if (schemaRow < self->schemaRowIndex) {
      self->schemaRowIndex = schemaRow;
   }
   else if (schemaRow > self->schemaRowIndex + 1) {
      self->schemaRowIndex = schemaRow - 1;
   }
}

void mapEditorRenderSchemas(MapEditor *self, Frame *frame)  {
   
   GridManager *gm = self->view->gridManager;
   int count = (int)gridManagerGetSchemaCount(gm);
   int x, y;

   int i = self->schemaRowIndex * SCHEMA_COLUMNS;

   for (y = 0; y < SCHEMA_ROWS && i < count; ++y) {
      short renderY = SCHEMA_TOP + (y * GRID_CELL_SIZE);

      for (x = 0; x < SCHEMA_COLUMNS && i < count; ++x) {
         short renderX = SCHEMA_LEFT + (x * GRID_CELL_SIZE);

         gridManagerRenderSchema(gm, i, frame, FrameRegionFULL, renderX, renderY);

         if (i == self->schemaIndex) {
            //hjighlight the selected schema
            frameRenderLineRect(frame, FrameRegionFULL,
               renderX, renderY,
               renderX + GRID_CELL_SIZE - 1, renderY + GRID_CELL_SIZE - 1, 15);
         }

         ++i;
      }
   }

   
}

void mapEditorRenderXYDisplay(MapEditor *self, Frame *frame) {
   textAreaRender(self->xyDisplay, self->view, frame);
}

void mapEditorUpdateStats(MapEditor *self, Int2 mouseGridPos) {
   if (self->mouseGridPos.x != mouseGridPos.x || self->mouseGridPos.y != mouseGridPos.y) {
      char buff[128];
      sprintf(buff, "X: [c=0,5]%d[/c]\nY: [c=0,5]%d[/c]", mouseGridPos.x, mouseGridPos.y);
      textAreaSetText(self->xyDisplay, buff);
      self->mouseGridPos = mouseGridPos;      
   }
}


