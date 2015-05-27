#include "CoreComponents.h"

#define ComponentT GridComponent
#include "Entities\ComponentImpl.h"

#define ComponentT SizeComponent
#include "Entities\ComponentImpl.h"

#define ComponentT TeamComponent
#include "Entities\ComponentImpl.h"

#define ComponentT PositionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT LockedPositionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT ImageComponent
#include "Entities\ComponentImpl.h"

#define ComponentT LayerComponent
#include "Entities\ComponentImpl.h"

#define ComponentT InterpolationComponent
#include "Entities\ComponentImpl.h"

#define ComponentT WanderComponent
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

static void TextComponentDestroy(TextComponent *self){
   stringDestroy(self->text);
}

#define COMP_DESTROY_FUNC TextComponentDestroy
#define ComponentT TextComponent
#include "Entities\ComponentImpl.h"

#define ComponentT CommandComponent
#include "Entities\ComponentImpl.h"

#define ComponentT AbilitySlotsComponent
#include "Entities\ComponentImpl.h"

#define ComponentT DestructionComponent
#include "Entities\ComponentImpl.h"

#define ComponentT AIComponent
#include "Entities\ComponentImpl.h"

#define ComponentT DamageMarkerComponent
#include "Entities\ComponentImpl.h"