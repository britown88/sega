#pragma once

typedef struct EntitySystem_t EntitySystem;

typedef struct{
   int ID;
   EntitySystem *system;
}Entity;

EntitySystem *entitySystemCreate();
void entitySystemDestroy(EntitySystem *self);

Entity entitySystemCreateEntity(EntitySystem *self);
void entitySystemDestroyEntity(EntitySystem *self, int entityID);