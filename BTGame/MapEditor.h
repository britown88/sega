#pragma once

#include "WorldView.h"

typedef struct MapEditor_t MapEditor;

MapEditor *mapEditorCreate(WorldView *view);
void mapEditorDestroy(MapEditor *self);

void mapEditorInitialize(MapEditor *self);
void mapEditorSetEnabled(MapEditor *self, bool enabled);
