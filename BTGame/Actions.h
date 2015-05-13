#pragma once

#include "Entities\Entities.h"
#include "segautils\Coroutine.h"

typedef struct CommandManager_t CommandManager;

typedef Entity Action;
typedef Action *ActionPtr;

#define VectorTPart ActionPtr
#include "segautils\Vector_Decl.h"

typedef struct {
   Entity *user;
}UserComponent;
#define ComponentT UserComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int x, y;
}TargetPositionComponent;
#define ComponentT TargetPositionComponent
#include "Entities\ComponentDecl.h"


//put your action creating function decls here
Action *createActionGridMove(CommandManager *self, int x, int y);