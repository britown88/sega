#pragma once

#include "segashared/Strings.h"

typedef struct DB_t DB;
typedef struct WorldView_t WorldView;

DB *DBCreate(WorldView *view);
int DBConnect(DB *self, const char *filename);
int DBDisconnect(DB *self);
void DBDestroy(DB *self);

const char *DBGetError(DB *self);

int DBInsertImage(DB *self, StringView id, const char *path);
int DBSelectImage(DB *self, StringView id, byte **buffer, int *size);





