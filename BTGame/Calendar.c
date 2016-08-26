#include "Calendar.h"
#include "WorldView.h"
#include "segashared/CheckedMemory.h"
#include "GameClock.h"
#include "segalib/EGA.h"
#include "ImageLibrary.h"

struct Calendar_t {
   WorldView *view;

   Minute current, target, tickLength;

   Microseconds startTime;
   Milliseconds tickDelay;

   ManagedImage *clock;
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


Calendar *calendarCreate(WorldView *view) {
   Calendar *out = checkedCalloc(1, sizeof(Calendar));
   out->view = view;
   
   out->tickDelay = 500;   
   out->clock = imageLibraryGetImage(view->imageLibrary, stringIntern("clock"));

   return out;
}
void calendarDestroy(Calendar *self) {
   managedImageDestroy(self->clock);
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
   frameRenderImage(frame, FrameRegionFULL, 96, 0, managedImageGetImage(self->clock));
}

void calendarUpdate(Calendar *self) {
   Microseconds t = gameClockGetTime();

   if (self->startTime == 0) {
      self->startTime = gameClockGetTime();
   }

   while (t_u2m(t - self->startTime) >= self->tickDelay) {
      self->startTime += t_m2u(self->tickDelay);
      self->target += self->tickLength;
   }

   while (self->current < self->target) {
      ++self->current;

      //here's where we can run events!
   }
}