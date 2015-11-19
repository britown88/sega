#pragma once

#include "Entities\Entities.h"
#include "segashared\Strings.h"
#include "MeshRendering.h"
#include "segautils\String.h"
#include "segautils/Time.h"

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
   LayerGrid,
   LayerGridLighting,
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
   String *text;
}TextLine;

#define VectorTPart TextLine
#include "segautils/Vector_Decl.h"

typedef struct {
   vec(TextLine) *lines;
   byte fg, bg;
}TextComponent;

#define ComponentT TextComponent
#include "Entities\ComponentDecl.h"

//Rendering treats the position component as WorldPosition
//Entity is then drawn relative to viewport
typedef struct {
   EMPTY_STRUCT;
}InViewComponent;

#define ComponentT InViewComponent
#include "Entities\ComponentDecl.h"

//Marks an entity as Rendererd UI (the rendermanager will always draw these if it can!)
typedef struct {
   EMPTY_STRUCT;
}RenderedUIComponent;

#define ComponentT RenderedUIComponent
#include "Entities\ComponentDecl.h"

//light will treat positioncomponent as worldposition
typedef struct {
   byte radius;
   byte centerLevel;
   byte fadeWidth; // number of tiles devoted to fading out... actual radius will be adjusted toa  minimum of this
}LightComponent;

#define ComponentT LightComponent
#include "Entities\ComponentDecl.h"

//Marks an entity as Rendererd UI (the rendermanager will always draw these if it can!)
typedef struct {
   EMPTY_STRUCT;
}GriddedComponent;

#define ComponentT RenderedUIComponent
#include "Entities\ComponentDecl.h"


typedef struct {
   int x, y;
}GridComponent;

#define ComponentT GridComponent
#include "Entities\ComponentDecl.h"

typedef struct {
   int destX, destY;
   Milliseconds time;
   Microseconds overflow;//if this is not 0, the interpolation finished but there's leftover time to apply to the next go
}InterpolationComponent;

#define ComponentT InterpolationComponent
#include "Entities\ComponentDecl.h"



#pragma pack(pop)