#pragma once

#include "Input.h"

typedef struct {
   SegaKeyboardEvents event;
   SegaKeys key;
}KeyboardEvent;

typedef struct Keyboard_t Keyboard;

Keyboard *keyboardCreate();
void keyboardDestroy(Keyboard *self);

void keyboardPushEvent(Keyboard *self, KeyboardEvent *event);

//return if succeeded (false if empty)
int keyboardPopEvent(Keyboard *self, KeyboardEvent *eventOut);
int keyboardIsDown(Keyboard *self, SegaKeys key);
void keyboardFlushQueue(Keyboard *self);

