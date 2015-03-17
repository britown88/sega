#include "Input.h"

#include "segashared\CheckedMemory.h"

#define VectorT MouseEvent
#include "segautils\Vector_Create.h"

#define ClosureTPart \
    CLOSURE_RET(Int2) \
    CLOSURE_NAME(MousePos) \
    CLOSURE_ARGS()
#include "segautils\Closure_Impl.h"

typedef struct Mouse_t {
   vec(MouseEvent) *eventQueue;
   MousePos getPos;
   int heldMap[SegaMouseBtn_COUNT];
   int queuePos;
};

Mouse *mouseCreate(MousePos posFunc){
    Mouse *out = checkedCalloc(1, sizeof(Mouse));
    out->eventQueue = vecCreate(MouseEvent)(NULL);
    out->getPos = posFunc;
    return out;
 }
 void mouseDestroy(Mouse *self){
    closureDestroy(MousePos)(&self->getPos);
    vecDestroy(MouseEvent)(self->eventQueue);
    checkedFree(self);
 }

 Int2 mouseGetPosition(Mouse *self){
    return closureCall(&self->getPos);
 }

 void mousePushEvent(Mouse *self, MouseEvent *event){
    vecPushBack(MouseEvent)(self->eventQueue, event);
 }

//return if succeeded (false if empty)
 int mousePopEvent(Mouse *self, MouseEvent *eventOut){
    if (self->queuePos == vecSize(MouseEvent)(self->eventQueue)){
       return false;
    }

    *eventOut = *vecAt(MouseEvent)(self->eventQueue, self->queuePos++);

    if (eventOut->action == SegaKey_Pressed){
       self->heldMap[eventOut->button] = true;
    }
    else if (eventOut->action == SegaKey_Released){
       self->heldMap[eventOut->button] = false;
    }

    return true;
 }
 int mouseIsDown(Mouse *self, SegaMouseButtons button){
    return self->heldMap[button];
 }
 void mouseFlushQueue(Mouse *self){
    while (mousePopEvent(self, &(MouseEvent){0}));

    self->queuePos = 0;
    vecClear(MouseEvent)(self->eventQueue);
 }