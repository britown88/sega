#pragma once

#include "Managers.h"
#include "segautils\String.h"
#include <stdio.h>

typedef struct WorldView_t WorldView;

LogManager *createLogManager(WorldView *view);
void logManagerUpdate(LogManager *self);

void logManagerPushString(LogManager *self, String *text);

#define MAX_LOG_BUFFER_LEN 128

//use this for most pushes, has same syntax as sprintf
#define logManagerPushMessage(manager, ...) { \
   char __buffer[MAX_LOG_BUFFER_LEN] = {0};\
   String *__logOut; \
   sprintf_s(__buffer, MAX_LOG_BUFFER_LEN, __VA_ARGS__);\
   __logOut = stringCreate(__buffer); \
   logManagerPushString(manager, __logOut); \
}