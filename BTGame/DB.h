#pragma once

#include "segashared/Strings.h"

typedef struct DB_t DB;
typedef struct WorldView_t WorldView;

#define DB_SUCCESS 0
#define DB_FAILURE 1
#define DB_CREATED 2

DB *DBCreate(WorldView *view);
int DBConnect(DB *self, const char *filename);
int DBDisconnect(DB *self);
void DBDestroy(DB *self);

const char *DBGetError(DB *self);

int DBInsertImage(DB *self, StringView id, const char *path);
int DBSelectImage(DB *self, StringView id, byte **buffer, int *size);





