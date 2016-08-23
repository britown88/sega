#pragma once

#include "segautils/Time.h"
#include "segautils/Vector.h"

typedef struct WorldView_t WorldView;
typedef struct Frame_t Frame;

typedef struct Actor_t Actor;

typedef struct ActorManager_t ActorManager;

typedef Actor* ActorPtr;
#define VectorTPart ActorPtr
#include "segautils/Vector_Decl.h"

ActorManager *actorManagerCreate(WorldView *view);
void actorManagerDestroy(ActorManager *self);
Actor *actorManagerCreateActor(ActorManager *self);
void actorManagerUpdate(ActorManager *self);
void actorManagerClearErrorFlag(ActorManager *self);//resumes execution of script steps if it was halted by an error
void actorManagerRender(ActorManager *self, Frame *f);

void actorDestroy(Actor *self);

Int2 actorGetPosition(Actor *self);
void actorSetPosition(Actor *self, Int2 pos);

Milliseconds actorGetMoveTime(Actor *self);
Milliseconds actorGetMoveDelay(Actor *self);
void actorSetMoveTime(Actor *self, Milliseconds time);
void actorSetMoveDelay(Actor *self, Milliseconds delay);




