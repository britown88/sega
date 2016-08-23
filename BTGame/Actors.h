#pragma once

#include "segautils/Time.h"
#include "segautils/Vector.h"
#include "segashared/Strings.h"

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

Int2 actorGetGridPosition(Actor *self);
void actorSetGridPosition(Actor *self, Int2 pos);

void actorSnap(Actor *self);

//helper to combine grid pos with offsets
Int2 actorGetWorldPosition(Actor *self);

Milliseconds actorGetMoveTime(Actor *self);
Milliseconds actorGetMoveDelay(Actor *self);
void actorSetMoveTime(Actor *self, Milliseconds time);
void actorSetMoveDelay(Actor *self, Milliseconds delay);

void actorSetImage(Actor *self, StringView imgId);
void actorSetImagePos(Actor *self, Int2 imgPos);

typedef struct GridToken_t GridToken;
GridToken *actorGetGridToken(Actor *self);

typedef struct LightSource_t LightSource;
LightSource *actorGetLightSource(Actor *self);





