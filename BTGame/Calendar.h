#pragma once

#include "segautils/Defs.h"
#include "segautils/Time.h"

#define SEASON_PER_YEAR 4
#define MONTHS_PER_SEASON 2
#define WEEKS_PER_MONTH 4
#define DAYS_PER_WEEK 10
#define HOURS_PER_DAY 24
#define MINUTES_PER_HOUR 60
#define FUCK_SECONDS

#define MINUTES_PER_DAY (MINUTES_PER_HOUR * HOURS_PER_DAY)
#define MINUTES_PER_WEEK (MINUTES_PER_DAY * DAYS_PER_WEEK)
#define MINUTES_PER_MONTH (MINUTES_PER_WEEK * WEEKS_PER_MONTH)
#define MINUTES_PER_SEASON (MINUTES_PER_MONTH * MONTHS_PER_SEASON)
#define MINUTES_PER_YEAR (MINUTES_PER_SEASON * SEASON_PER_YEAR)

typedef size_t Year;
typedef size_t Season;
typedef size_t Month;
typedef size_t Week;
typedef size_t Day;
typedef size_t Hour;
typedef size_t Minute;

typedef struct {
   Year year;
   Season season;
   Month month;
   Week week;
   Day day;
}Date;

typedef struct {
   Hour hour;
   Minute minute;
}Time;

typedef struct {
   Date date;
   Time time;
}DateTime;

typedef struct Calendar_t Calendar;
typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;

Calendar *calendarCreate(WorldView *view);
void calendarDestroy(Calendar *self);

void calendarUpdate(Calendar *self);

void calendarSetDateTIme(Calendar *self, DateTime dt);
void calendarJump(Calendar *self, DateTime dt);

DateTime calendarGetDateTime(Calendar *self);
Date calendarGetDate(Calendar *self);
Time calendarGetTime(Calendar *self);

Time calendarGetSunrise(Calendar *self);
Time calendarGetSunset(Calendar *self);

void calendarSetTickDelay(Calendar *self, Milliseconds msPerTick);
void calendarSetTickLength(Calendar *self, DateTime tick);

void calendarRenderClock(Calendar *self, Frame *frame);


