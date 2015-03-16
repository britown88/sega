#pragma once

#include "Input.h"
#include "segautils\DLLBullshit.h"

typedef struct {
   SegaKeyboardEvents event;
   SegaKeys key;
}KeyboardEvent;

typedef struct Keyboard_t Keyboard;

DLL_PUBLIC Keyboard *keyboardCreate();
DLL_PUBLIC void keyboardDestroy(Keyboard *self);

DLL_PUBLIC void keyboardPushEvent(Keyboard *self, KeyboardEvent *event);

//return if succeeded (false if empty)
DLL_PUBLIC int keyboardPopEvent(Keyboard *self, KeyboardEvent *eventOut);
DLL_PUBLIC int keyboardIsDown(Keyboard *self, SegaKeys key);
DLL_PUBLIC void keyboardFlushQueue(Keyboard *self);

