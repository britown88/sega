#include "GameHelpers.h"
#include "Viewport.h"

#include "ImageLibrary.h"

Int2 screenToWorld(WorldView *view, Int2 pos) {
   Viewport *vp = view->viewport;

   return (Int2) { 
      .x = (pos.x - vp->region.origin_x + vp->worldPos.x), 
      .y = (pos.y - vp->region.origin_y + vp->worldPos.y) 
   };
}