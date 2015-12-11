#include "MapEditor.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "GridManager.h"
#include "RichText.h"
#include "CoreComponents.h"

#include <stdio.h>

#define STATS_LEFT 2
#define STATS_TOP 2

#define SCHEMA_LEFT 41
#define SCHEMA_TOP 165
#define SCHEMA_COLUMNS 27
#define SCHEMA_ROWS 2

struct MapEditor_t {
   WorldView *view;
   bool enabled;

   int schemaIndex;

   Int2 mouseGridPos;
   Entity *statsEntity;
   
   RichText *rt;
   vec(RichTextLine) *rtLines;
};

static void _createStatsEntity(MapEditor *self) {
   //self->statsEntity = entityCreate(self->view->entitySystem);

   Entity *e = entityCreate(self->view->entitySystem);
   TextComponent tc = { .lines = vecCreate(TextLine)(&textLineDestroy) };

   vecPushBack(TextLine)(tc.lines, &(TextLine){
      .x = STATS_LEFT, .y = STATS_TOP, 
      .line = vecCreate(Span)(&spanDestroy)
   });

   vecPushBack(TextLine)(tc.lines, &(TextLine){
      .x = STATS_LEFT, .y = STATS_TOP + 1,
      .line = vecCreate(Span)(&spanDestroy)
   });

   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   COMPONENT_ADD(e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(e, &tc);
   entityUpdate(e);

   self->statsEntity = e;
}

MapEditor *mapEditorCreate(WorldView *view) {
   MapEditor *out = checkedCalloc(1, sizeof(MapEditor));
   out->view = view;

   out->rt = richTextCreateFromRaw("");
   out->rtLines = vecCreate(RichTextLine)(&richTextLineDestroy);

   _createStatsEntity(out);

   return out;
}
void mapEditorDestroy(MapEditor *self) {

   richTextDestroy(self->rt);
   vecDestroy(RichTextLine)(self->rtLines);

   checkedFree(self);
}

void mapEditorInitialize(MapEditor *self) {

}
void mapEditorSetEnabled(MapEditor *self, bool enabled) {
   self->enabled = enabled;
   entityGet(VisibilityComponent)(self->statsEntity)->shown = enabled;
}

void mapEditorRender(MapEditor *self, Frame *frame) {
   
   GridManager *gm = self->view->managers->gridManager;
   int count = (int)gridManagerGetSchemaCount(gm);
   int x, y, i = 0;

   for (y = 0; y < SCHEMA_ROWS && i < count; ++y) {
      short renderY = SCHEMA_TOP + (y * GRID_CELL_SIZE);

      for (x = 0; x < SCHEMA_COLUMNS && i < count; ++x) {
         short renderX = SCHEMA_LEFT + (x * GRID_CELL_SIZE);

         gridManagerRenderSchema(gm, i, frame, FrameRegionFULL, renderX, renderY);

         if (i == self->schemaIndex) {
            frameRenderLine(frame, FrameRegionFULL, renderX, renderY, renderX + GRID_CELL_SIZE, renderY, 15);
            frameRenderLine(frame, FrameRegionFULL, renderX + GRID_CELL_SIZE, renderY, renderX + GRID_CELL_SIZE, renderY + GRID_CELL_SIZE, 15);
            frameRenderLine(frame, FrameRegionFULL, renderX + GRID_CELL_SIZE, renderY + GRID_CELL_SIZE, renderX, renderY + GRID_CELL_SIZE, 15);
            frameRenderLine(frame, FrameRegionFULL, renderX, renderY + GRID_CELL_SIZE, renderX, renderY, 15);
         }

         ++i;
      }
   }
}

void mapEditorUpdateStats(MapEditor *self, Int2 mouseGridPos) {
   if (self->mouseGridPos.x != mouseGridPos.x || self->mouseGridPos.y != mouseGridPos.y) {
      TextComponent *tc = entityGet(TextComponent)(self->statsEntity);
      TextLine *xLine = vecAt(TextLine)(tc->lines, 0);
      TextLine *yLine = vecAt(TextLine)(tc->lines, 1);

      char buff[128];

      sprintf(buff, "X: [c=0,5]%d[/c]\nY: [c=0,5]%d[/c]", mouseGridPos.x, mouseGridPos.y);

      richTextResetFromRaw(self->rt, buff);

      vecClear(RichTextLine)(self->rtLines);
      richTextRenderToLines(self->rt, 0, self->rtLines);

      richTextLineCopy(*vecAt(RichTextLine)(self->rtLines, 0), xLine->line);
      richTextLineCopy(*vecAt(RichTextLine)(self->rtLines, 1), yLine->line);

      self->mouseGridPos = mouseGridPos;
   }
}


