#pragma once

#include "WorldView.h"

typedef struct MapEditor_t MapEditor;
typedef struct Texture_t Texture;

MapEditor *mapEditorCreate(WorldView *view);
void mapEditorDestroy(MapEditor *self);

void mapEditorRenderSchemas(MapEditor *self, Texture *tex);
void mapEditorRenderXYDisplay(MapEditor *self, Texture *tex);

void mapEditorUpdateStats(MapEditor *self, Int2 mouseGridPos);
void mapEditorReset(MapEditor *self);

bool mapEditorPointInSchemaWindow(MapEditor *self, Int2 p);
void mapEditorSelectSchema(MapEditor *self, Int2 mousePos);
void mapEditorScrollSchemas(MapEditor *self, int deltaY);

byte mapEditorGetSelectedSchema(MapEditor *self);
void mapEditorSetSelectedSchema(MapEditor *self, byte schema);

