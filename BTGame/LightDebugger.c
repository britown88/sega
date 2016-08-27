#include "LightDebugger.h"
#include "segashared/CheckedMemory.h"
#include "segalib/EGA.h"
#include "Viewport.h"
#include "WorldView.h"
#include "GameClock.h"

typedef struct {
   Int2 p1; 
   Int2 p2; 
   bool blocked;
}Ray;

#define VectorT Ray
#include "segautils/Vector_Create.h"

struct LightDebugger_t {
   WorldView *view;
   bool enabled;
   Recti source;
   Recti target;
   vec(Ray) *rays;

   Microseconds startTime;
};

LightDebugger *lightDebuggerCreate(WorldView *view) {
   LightDebugger *out = checkedCalloc(1, sizeof(LightDebugger));
   out->rays = vecCreate(Ray)(NULL);
   out->view = view;
   return out;
}
void lightDebuggerDestroy(LightDebugger *self) {
   vecDestroy(Ray)(self->rays);
   checkedFree(self);
}

void lightDebuggerStartNewSet(LightDebugger *self, Recti source, Recti target) {
   self->enabled = true;
   vecClear(Ray)(self->rays);
   self->source = source;
   self->target = target;
   self->startTime = gameClockGetTime();

}
void lightDebuggerAddRay(LightDebugger *self, Int2 p1, Int2 p2, bool blocked) {
   vecPushBack(Ray)(self->rays, &(Ray) { {p1.x, p1.y}, { p2.x, p2.y }, blocked});
}
void lightDebuggerClear(LightDebugger *self) {
   self->enabled = false;
   
}

void lightDebuggerRender(LightDebugger *self, Frame *frame) {
   if (self->enabled) {
      byte rectColor = 7;
      Viewport *vp = self->view->viewport;
      int vpx = vp->worldPos.x;
      int vpy = vp->worldPos.y;
      Milliseconds delta = t_u2m(gameClockGetTime() - self->startTime);
      static const Milliseconds period = 250;
      bool showAll = (delta % (period << 1)) >= period;
      size_t rayCount = vecSize(Ray)(self->rays);

      frameRenderLineRect(frame, &vp->region, self->source.left-vpx, self->source.top - vpy, self->source.right - vpx, self->source.bottom - vpy, 6);
      frameRenderLineRect(frame, &vp->region, self->target.left - vpx, self->target.top - vpy, self->target.right - vpx, self->target.bottom - vpy, 7);

      if (rayCount > 0) {
         size_t rayIndex = (delta / (period << 1)) % rayCount;

         if (true) {
            size_t i = 0;
            for (i = 0; i < rayIndex; ++i) {
               Ray *r = vecAt(Ray)(self->rays, i);
               frameRenderLine(frame, &vp->region, r->p1.x - vpx, r->p1.y - vpy, r->p2.x - vpx, r->p2.y - vpy, r->blocked ? 9 : 5);
            }
         }

         {
            Ray *r = vecAt(Ray)(self->rays, rayIndex);
            frameRenderLine(frame, &vp->region, r->p1.x - vpx, r->p1.y - vpy, r->p2.x - vpx, r->p2.y - vpy, r->blocked ? 9 : 5);
         }
         
      }

   }
}
