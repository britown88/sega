#include "CoreComponents.h"

#define ComponentT SizeComponent
#include "Entities\ComponentImpl.h"

#define ComponentT PositionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ImageComponent
#include "Entities\ComponentImpl.h"

#define ComponentT LayerComponent
#include "Entities\ComponentImpl.h"

#define ComponentT MeshComponent
#include "Entities\ComponentImpl.h"

static void PolygonComponentDestroy(PolygonComponent *self){
   vecDestroy(Int2)(self->pList);
}

#define COMP_DESTROY_FUNC PolygonComponentDestroy
#define ComponentT PolygonComponent
#include "Entities\ComponentImpl.h"

#define ComponentT RectangleComponent
#include "Entities\ComponentImpl.h"

#define ComponentT VisibilityComponent
#include "Entities\ComponentImpl.h"

#define VectorTPart TextLine
#include "segautils/Vector_Impl.h"

void textLineDestroy(TextLine *self) {
   if (self->text) {
      stringDestroy(self->text);
   }
}

static void TextComponentDestroy(TextComponent *self){
   vecDestroy(TextLine)(self->lines);
}

#define COMP_DESTROY_FUNC TextComponentDestroy
#define ComponentT TextComponent
#include "Entities\ComponentImpl.h"

#define ComponentT InViewComponent
#include "Entities\ComponentImpl.h"

#define ComponentT RenderedUIComponent
#include "Entities\ComponentImpl.h"

#define ComponentT LightComponent
#include "Entities\ComponentImpl.h"

#define ComponentT GridComponent
#include "Entities\ComponentImpl.h"

#define ComponentT InterpolationComponent
#include "Entities\ComponentImpl.h"

