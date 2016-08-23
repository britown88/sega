#include "Weather.h"
#include "WorldView.h"
#include "GameClock.h"
#include "Viewport.h"
#include "segalib/EGA.h"
#include "segautils/Time.h"
#include "SEGA/App.h"
#include "GameClock.h"

#include "segashared/CheckedMemory.h"

typedef struct {
   Microseconds startTime;
   int distance;
   Float2 slope;
   Int2 startPos;
   Milliseconds speed;//ms per pixel of distance
   byte color;
   int length;
}Raindrop;

#define VectorT Raindrop
#include "segautils/Vector_Create.h"

struct Weather_t {
   WorldView *view;

   vec(Raindrop) *rain;
};

void testRain(Weather *self) {
   Float3 n = vNormalized((Float3){-1.0f, 3.0f, 0.0f});
   Float2 n2 = { n.x, n.y };
   Microseconds speed = t_m2u(1000);

   int left = self->view->viewport->worldPos.x - 100;
   int top = self->view->viewport->worldPos.y - 100;
   int right = left + self->view->viewport->region.width + 100;
   int bottom = top + self->view->viewport->region.height + 100;

   App *app = appGet();

   for (int i = 0; i < 500; ++i) {
      Raindrop r = {
         .startTime = gameClockGetTime(self->view->gameClock),
         .distance = appRand(app, 50, 150),
         .slope = {n.x, n.y},
         .startPos = {appRand(app, left, right), appRand(app, top, bottom)},
         .speed = 10,
         .color = 7,
         .length = 15
      };
      vecPushBack(Raindrop)(self->rain, &r);
   }
}

Weather *createWeather(WorldView *view) {
   Weather *out = checkedCalloc(1, sizeof(Weather));
   out->view = view;

   out->rain = vecCreate(Raindrop)(NULL);

   return out;
}
void weatherDestroy(Weather *self) {
   vecDestroy(Raindrop)(self->rain);
   checkedFree(self);
}

static void _renderRaindrop(Weather *self, Raindrop *r, Frame *frame) {
   Microseconds currentTime = gameClockGetTime(self->view->gameClock);

   if (currentTime - r->startTime > t_m2u(r->distance * r->speed)) {
      //reboot
      int left = self->view->viewport->worldPos.x - 100;
      int top = self->view->viewport->worldPos.y - 100;
      int right = left + self->view->viewport->region.width + 100;
      int bottom = top + self->view->viewport->region.height + 100;
      App *app = appGet();

      r->startPos = (Int2) { appRand(app, left, right), appRand(app, top, bottom) };
      r->distance = appRand(app, 50, 150);
      r->startTime = currentTime;
   }
   else {
      float d = t_u2s(currentTime - r->startTime) / t_m2s(r->distance*r->speed);
      int x0 = r->startPos.x + r->distance * r->slope.x * d;
      int y0 = r->startPos.y + r->distance * r->slope.y * d;
      int x1 = x0 + r->slope.x  + r->slope.x * r->length * d;
      int y1 = y0 + r->slope.y + r->slope.y * r->length * d;
      int vpx = self->view->viewport->worldPos.x;
      int vpy = self->view->viewport->worldPos.y;

      frameRenderLine(frame, &self->view->viewport->region,
          x0 - vpx, y0 - vpy, x1 - vpx, y1 - vpy, r->color );
   }
}

void weatherRender(Weather *self, Frame *frame) {
   FrameRegion *region = &self->view->viewport->region;
   vecForEach(Raindrop, r, self->rain, {
      _renderRaindrop(self, r, frame);
   });

}