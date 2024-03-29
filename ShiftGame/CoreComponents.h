#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"
#include "MeshRendering.h"
#include "Actions.h"
#include "segautils\String.h"
#include "segautils/Time.h"

#pragma pack(push, 1)

typedef struct {
   int x, y;
}GridComponent;

#define ComponentT GridComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int x, y;
}SizeComponent;

#define ComponentT SizeComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   size_t teamID;
}TeamComponent;

#define ComponentT TeamComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int x, y;
}PositionComponent;

#define ComponentT PositionComponent
#include "Entities\ComponentDecl.h"

//used to lock an entity's position to another's
//updated in interpolation manager
typedef struct {
   Entity *parent;
   int offsetX, offsetY;
}LockedPositionComponent;

#define ComponentT LockedPositionComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   StringView filename;
   bool partial;
   short x, y, width, height;
}ImageComponent;

#define ComponentT ImageComponent
#include "Entities\ComponentDecl.h"

typedef enum{
   LayerBackground,
   LayerSubToken0,
   LayerTokens,
   LayerPostToken0,
   LayerPostToken1,
   LayerPostTokenDmg,
   LayerUI,
   LayerCursor,
   LayerCount
} Layer;

typedef struct{
   Layer layer;
}LayerComponent;

#define ComponentT LayerComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int destX, destY;
   Seconds time;
}InterpolationComponent;

#define ComponentT InterpolationComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   EMPTY_STRUCT;
}WanderComponent;

#define ComponentT WanderComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   vec(Vertex) *vbo;
   vec(size_t) *ibo;
   int size;
   Float3 rotNormal;
   float angle;
}MeshComponent;

#define ComponentT MeshComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   vec(Int2) *pList;
   byte color;
   bool open;
}PolygonComponent;

#define ComponentT PolygonComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   byte color;
}RectangleComponent;

#define ComponentT RectangleComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   bool shown;
}VisibilityComponent;

#define ComponentT VisibilityComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   byte x, y;
   byte fg, bg;
   String *text;
}TextComponent;

#define ComponentT TextComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   vec(ActionPtr) *actions;
   CoroutineRequest request;
}CommandComponent;

#define ComponentT CommandComponent
#include "Entities\ComponentDecl.h"

#define ABILITY_SLOT_COUNT 2

typedef struct {
   StringView slots[ABILITY_SLOT_COUNT];
} AbilitySlotsComponent;

#define ComponentT AbilitySlotsComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   EMPTY_STRUCT;
} DestructionComponent;

#define ComponentT DestructionComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   EMPTY_STRUCT;
} AIComponent;

#define ComponentT AIComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   size_t value;
} DamageMarkerComponent;

#define ComponentT DamageMarkerComponent
#include "Entities\ComponentDecl.h"

#pragma pack(pop)