#include "MapEditor.h"
#include "Managers.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"

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


