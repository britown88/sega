#include "Calendar.h"
#include "WorldView.h"
#include "segashared/CheckedMemory.h"
#include "GameClock.h"
#include "segalib/EGA.h"
#include "ImageLibrary.h"

#include <math.h>

#define CALENDAR_TOP 0
#define CALENDAR_LEFT 96
#define CALENDAR_WIDTH 128
#define CALENDAR_HEIGHT 11

static const Int2 RisePosition = { 6, 9 };
static const Int2 NoonPosition = { 64, -1 };
static const Int2 SetPosition = { 121, 8 };

Int2 circleCenterFromPoints(Int2 a, Int2 b, Int2 c) {
   int yDeltaA = b.y - a.y;
   int xDeltaA = b.x - a.x;
   int yDeltaB = c.y - b.y;
   int xDeltaB = c.x - b.x;

   float aSlope = yDeltaA / (float)xDeltaA;
   float bSlope = yDeltaB / (float)xDeltaB;

   float centerX = (aSlope * bSlope * (a.y - c.y) + bSlope * (a.x + b.x) - aSlope * (b.x + c.x)) / (2.0f * (bSlope - aSlope));
   float centerY = -1.0f * (centerX - (a.x + b.x) / 2.0f) / aSlope + (a.y + b.y) / 2.0f;

   return (Int2) {(int)centerX, (int)centerY};
}

struct Calendar_t {
   WorldView *view;

   Minute current, target, tickLength;

   Microseconds startTime;
   Milliseconds tickDelay;

   ManagedImage *clock, *sun, *moon;
   FrameRegion clockRegion;

   Int2 circleCenter;
   float circleRadius;
   float periodAngle;
   float riseAngle;


};

Time timeFromMinute(Minute m) {
   Time out = { 0 };
   out.minute = m % MINUTES_PER_HOUR;
   out.hour = (m / MINUTES_PER_HOUR) % HOURS_PER_DAY;
   return out;
}

Date dateFromMinute(Minute m) {
   Date out = { 0 };

   out.day = (m / MINUTES_PER_DAY) % DAYS_PER_WEEK;
   out.week = (m / MINUTES_PER_WEEK) % WEEKS_PER_MONTH;
   out.month = (m / MINUTES_PER_MONTH) % MONTHS_PER_SEASON;
   out.season = (m / MINUTES_PER_SEASON) % SEASON_PER_YEAR;
   out.year = m / MINUTES_PER_YEAR;
   
   return out;
}

DateTime dateTimeFromMinute(Minute m) {
   DateTime out = { 0 };
   out.time = timeFromMinute(m);
   out.date = dateFromMinute(m);
   return out;   
}

Minute minuteFromDateTime(DateTime *dt) {
   return dt->time.minute +
      dt->time.hour * MINUTES_PER_HOUR +
      dt->date.day * MINUTES_PER_DAY +
      dt->date.week * MINUTES_PER_WEEK +
      dt->date.month * MINUTES_PER_MONTH +
      dt->date.season * MINUTES_PER_SEASON +
      dt->date.year * MINUTES_PER_YEAR;
}

Minute minuteFromTime(Time dt) {
   return dt.minute + dt.hour * MINUTES_PER_HOUR;
}

float dist(Int2 a, Int2 b) {
   return sqrtf((float)(powl(a.x - b.x, 2) + powl(a.y - b.y, 2)));
}

static void _buildCycleCircle(Calendar *self) {
   float arcDist = dist(RisePosition, SetPosition);
   Int2 p1 = { RisePosition.x, -RisePosition.y};
   Int2 p2 = { NoonPosition.x, -NoonPosition.y };
   Int2 p3 = { SetPosition.x, -SetPosition.y };

   Int2 c = circleCenterFromPoints(p1, p2, p3);
   float r = dist(p1, c);
   float morningAngle = asinf((c.x - p1.x) / r);
   float eveningAngle = asinf((p3.x - c.x) / r);
   float a = morningAngle + eveningAngle;

   self->circleCenter = (Int2){c.x, -c.y};
   self->circleRadius = r;
   self->riseAngle = morningAngle + 1.5708f;//+90degrees
   self->periodAngle = a;

}

Calendar *calendarCreate(WorldView *view) {
   Calendar *out = checkedCalloc(1, sizeof(Calendar));
   out->view = view;
   
   out->tickDelay = 25;   
   out->tickLength = 1;
   out->clock = imageLibraryGetImage(view->imageLibrary, stringIntern("clock"));
   out->sun = imageLibraryGetImage(view->imageLibrary, stringIntern("sun"));
   out->moon = imageLibraryGetImage(view->imageLibrary, stringIntern("moon"));
   out->clockRegion = (FrameRegion){ CALENDAR_LEFT, CALENDAR_TOP, CALENDAR_WIDTH, CALENDAR_HEIGHT };
   _buildCycleCircle(out);

   return out;
}
void calendarDestroy(Calendar *self) {
   managedImageDestroy(self->clock);
   managedImageDestroy(self->moon);
   managedImageDestroy(self->sun);
   checkedFree(self);
}

void calendarSetDateTIme(Calendar *self, DateTime dt) {
   self->current = self->target = minuteFromDateTime(&dt);
}
void calendarJump(Calendar *self, DateTime dt) {
   self->target += minuteFromDateTime(&dt);
}

DateTime calendarGetDateTime(Calendar *self) {
   return dateTimeFromMinute(self->target);
}
Date calendarGetDate(Calendar *self) {
   return dateFromMinute(self->target);
}
Time calendarGetTime(Calendar *self) {
   return timeFromMinute(self->target);
}

Time calendarGetSunrise(Calendar *self) {
   return (Time) { .hour = 6 };
}
Time calendarGetSunset(Calendar *self) {
   return (Time) { .hour = 18 };
}

void calendarSetTickDelay(Calendar *self, Milliseconds msPerTick) {
   self->tickDelay = msPerTick;
}
void calendarSetTickLength(Calendar *self, DateTime tick) {
   self->tickLength = minuteFromDateTime(&tick);
}

void calendarRenderClock(Calendar *self, Frame *frame) {
   Minute sunRise = minuteFromTime(calendarGetSunrise(self));
   Minute sunSet = minuteFromTime(calendarGetSunset(self));
   Minute currentTime = self->current % MINUTES_PER_DAY;

   bool night = currentTime < sunRise || currentTime >= sunSet;
   float delta = 0;

   if (night) {
      Minute nightLen = MINUTES_PER_DAY - (sunSet - sunRise);
      Minute nightTime = currentTime >= sunSet ? (currentTime - sunSet) : ((MINUTES_PER_DAY - sunSet) + currentTime);

      delta = nightTime / (float)(nightLen);
   }
   else {
      delta = (currentTime - sunRise) / (float)(sunSet - sunRise);
   }

   float angleSegement = self->periodAngle * delta;
   float a = -self->riseAngle + angleSegement;
   int x = self->circleCenter.x + (int)(self->circleRadius * cosf(a));
   int y = self->circleCenter.y + (int)(self->circleRadius * sinf(a));

   frameRenderImage(frame, &self->clockRegion, x - 3, y - 3, managedImageGetImage(night ? self->moon : self->sun));
   frameRenderImage(frame, &self->clockRegion, 0, 0, managedImageGetImage(self->clock));
}

void calendarUpdate(Calendar *self) {
   Microseconds t = gameClockGetTime();

   if (self->startTime == 0) {
      self->startTime = gameClockGetTime();
   }

   if (t_u2m(t - self->startTime) >= self->tickDelay) {
      self->startTime += t_m2u(self->tickDelay);

      self->target += self->tickLength;
   }

   while (self->current < self->target) {
      ++self->current;

      //here's where we can run events!
   }
}