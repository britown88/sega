#include "GameHelpers.h"
#include "Viewport.h"

#include "Entities/Entities.h"
#include "ImageLibrary.h"
#include "CoreComponents.h"

Int2 screenToWorld(WorldView *view, Int2 pos) {
   Viewport *vp = view->viewport;

   return (Int2) { 
      .x = (pos.x - vp->region.origin_x + vp->worldPos.x), 
      .y = (pos.y - vp->region.origin_y + vp->worldPos.y) 
   };
}

void changeBackground(WorldView *view, const char *file) {
   COMPONENT_LOCK(ImageComponent, ic, view->backgroundEntity, {
      ic->filename = stringIntern(file);
   });
}