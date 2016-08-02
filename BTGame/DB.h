#pragma once

typedef struct DB_t DB;
typedef struct WorldView_t WorldView;

DB *DBCreate(WorldView *view);
int DBConnect(DB *self, const char *filename);
int DBDisconnect(DB *self);
void DBDestroy(DB *self);

const char *DBGetError(DB *self);





