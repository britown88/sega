#pragma once

#include <stddef.h>

#define MAX_ENTITIES (1024 * 128)

typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;

EntitySystem *entitySystemCreate();
void entitySystemDestroy(EntitySystem *self);

Entity *entityCreate(EntitySystem *system);
void entityDestroy(Entity *self);
int entityGetID(Entity *self);
EntitySystem *entityGetSystem(Entity *self);


