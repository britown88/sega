#include "Keyboard.h"

#include "segashared\CheckedMemory.h"

#define VectorT KeyboardEvent
#include "segautils\Vector_Create.h"

typedef struct Keyboard_t {
   vec(KeyboardEvent) *eventQueue;
   int heldMap[SegaKey_COUNT];
   int queuePos;
};

Keyboard *keyboardCreate(){
   Keyboard *out = checkedCalloc(1, sizeof(Keyboard));
   out->eventQueue = vecCreate(KeyboardEvent)(NULL);
   return out;
}
void keyboardDestroy(Keyboard *self){
   vecDestroy(KeyboardEvent)(self->eventQueue);
   checkedFree(self);
}

void keyboardPushEvent(Keyboard *self, KeyboardEvent *event){
   vecPushBack(KeyboardEvent)(self->eventQueue, event);
}

//return if succeeded (false if empty)
int keyboardPopEvent(Keyboard *self, KeyboardEvent *eventOut){
   if (self->queuePos == vecSize(KeyboardEvent)(self->eventQueue)){
      return false;
   }

   *eventOut = *vecAt(KeyboardEvent)(self->eventQueue, self->queuePos++);

   if (eventOut->event == SegaKey_Pressed){
      self->heldMap[eventOut->key] == true;
   }
   else if (eventOut->event == SegaKey_Released){
      self->heldMap[eventOut->key] == false;
   }

   return true;
}
int keyboardIsDown(Keyboard *self, SegaKeys key){
   return self->heldMap[key];
}
void keyboardFlushQueue(Keyboard *self){
   while (keyboardPopEvent(self, &(KeyboardEvent){0}));

   self->queuePos = 0;
   vecClear(KeyboardEvent)(self->eventQueue);
}