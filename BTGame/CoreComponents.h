#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"
#include "MeshRendering.h"
#include "segautils\String.h"

#pragma pack(push, 1)

typedef struct {
   int x, y;
}SizeComponent;

#define ComponentT SizeComponent
#include "Entities\ComponentDecl.h"


typedef struct {
   int x, y;
}PositionComponent;

#define ComponentT PositionComponent
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

#pragma pack(pop)