#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"
#include "MeshRendering.h"
#include "Actions.h"

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
}LockedPositionComponent;

#define ComponentT LockedPositionComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   StringView filename;
}ImageComponent;

#define ComponentT ImageComponent
#include "Entities\ComponentDecl.h"

typedef enum{
   LayerBackground,
   LayerSubToken0,
   LayerTokens,
   LayerUI,
   LayerCount
} Layer;

typedef struct{
   Layer layer;
}LayerComponent;

#define ComponentT LayerComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int destX, destY;
   double time;
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
   StringView text;
}TextComponent;

#define ComponentT TextComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   vec(ActionPtr) *actions;
   bool cancelled;
}CommandComponent;

#define ComponentT CommandComponent
#include "Entities\ComponentDecl.h"

#pragma pack(pop)