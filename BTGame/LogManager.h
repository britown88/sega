#pragma once

#include "Managers.h"
#include <stdio.h>

LogManager *createLogManager(EntitySystem *system);
void logManagerUpdate(LogManager *self);

void logManagerPushStringView(LogManager *self, StringView text);

#define MAX_LOG_BUFFER_LEN 128

//use this for most pushes, has same syntax as sprintf
#define logManagerPushMessage(manager, ...) { \
   char __buffer[MAX_LOG_BUFFER_LEN] = {0};\
   sprintf_s(__buffer, MAX_LOG_BUFFER_LEN, __VA_ARGS__);\
   logManagerPushStringView(manager, stringIntern(__buffer)); \
}