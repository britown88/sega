#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"
#include "MeshRendering.h"

#pragma pack(push, 1)

typedef struct {
   int x, y;
}GridComponent;

#define ComponentT GridComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int x, y;
}PositionComponent;

#define ComponentT PositionComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   int x, y;
}VelocityComponent;

#define ComponentT VelocityComponent
#include "Entities\ComponentDecl.h"

typedef struct{
   StringView filename;
}ImageComponent;

#define ComponentT ImageComponent
#include "Entities\ComponentDecl.h"

typedef enum{
   LayerBackground,
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



#pragma pack(pop)