#pragma once

typedef struct EntitySystem_t EntitySystem;
typedef struct Entity_t Entity;

EntitySystem *entitySystemCreate();
void entitySystemDestroy(EntitySystem *self);

Entity *entityCreate(EntitySystem *system);
void entityDestroy(Entity *e);

