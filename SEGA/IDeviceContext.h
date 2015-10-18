#pragma once
#include "segashared/Strings.h"
#include "segautils/Vector.h"
#include "segautils\DLLBullshit.h"

typedef struct IDeviceContext_t IDeviceContext;
typedef struct Keyboard_t Keyboard;
typedef struct Mouse_t Mouse;

typedef struct {
   int(*init)(IDeviceContext*, int /*width*/, int /*height*/, StringView /*winName*/, int /*flags*/);
   void(*initRendering)(IDeviceContext*);
   void(*commitRender)(IDeviceContext*);
   void(*preRender)(IDeviceContext*);
   void(*postRender)(IDeviceContext*);
   int(*shouldClose)(IDeviceContext*);
   Int2(*windowSize)(IDeviceContext*);
   double(*time)(IDeviceContext*);
   Keyboard*(*keyboard)(IDeviceContext*);
   Mouse*(*mouse)(IDeviceContext*);
   void(*destroy)(IDeviceContext*);
} IDeviceContextVTable;

struct IDeviceContext_t{
   IDeviceContextVTable *vTable;
};

int iDeviceContextInit(IDeviceContext *self, int width, int height, StringView winTitle, int flags);
void iDeviceContextPreRender(IDeviceContext *self);
DLL_PUBLIC void iDeviceContextInitRendering(IDeviceContext *self);
DLL_PUBLIC void iDeviceContextCommitRender(IDeviceContext *self);
void iDeviceContextPostRender(IDeviceContext *self);
int iDeviceContextShouldClose(IDeviceContext *self);
Int2 iDeviceContextWindowSize(IDeviceContext *self);
double iDeviceContextTime(IDeviceContext *self);
Keyboard *iDeviceContextKeyboard(IDeviceContext *self);
Mouse *iDeviceContextMouse(IDeviceContext *self);
void iDeviceContextDestroy(IDeviceContext *self);