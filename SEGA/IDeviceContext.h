#pragma once
#include "segashared/Strings.h"
#include "segautils/Vector.h"

typedef struct IDeviceContext_t IDeviceContext;
typedef struct Keyboard_t Keyboard;

typedef struct {
   int(*init)(IDeviceContext*, int /*width*/, int /*height*/, StringView /*winName*/, int /*flags*/);
   void(*preRender)(IDeviceContext*);
   void(*postRender)(IDeviceContext*);
   int(*shouldClose)(IDeviceContext*);
   Int2(*windowSize)(IDeviceContext*);
   Float2(*pointerPos)(IDeviceContext*);
   int(*pointerEnabled)(IDeviceContext*);
   double(*time)(IDeviceContext*);
   Keyboard*(*keyboard)(IDeviceContext*);
   void(*destroy)(IDeviceContext*);
} IDeviceContextVTable;

struct IDeviceContext_t{
   IDeviceContextVTable *vTable;
};

int iDeviceContextInit(IDeviceContext *self, int width, int height, StringView winTitle, int flags);
void iDeviceContextPreRender(IDeviceContext *self);
void iDeviceContextPostRender(IDeviceContext *self);
int iDeviceContextShouldClose(IDeviceContext *self);
Int2 iDeviceContextWindowSize(IDeviceContext *self);
Float2 iDeviceContextPointerPos(IDeviceContext *self);
int iDeviceContextPointerEnabled(IDeviceContext *self);
double iDeviceContextTime(IDeviceContext *self);
Keyboard *iDeviceContextKeyboard(IDeviceContext *self);
void iDeviceContextDestroy(IDeviceContext *self);