#pragma once
#include "segashared/Strings.h"
#include "segautils/Vector.h"

typedef struct IDeviceContext_t IDeviceContext;
typedef struct IRenderer_t IRenderer;



typedef struct {
   int(*init)(IDeviceContext*, int /*width*/, int /*height*/, StringView /*winName*/, int /*flags*/);
   void(*preRender)(IDeviceContext*);
   void(*postRender)(IDeviceContext*);
   int(*shouldClose)(IDeviceContext*);
   Int2(*windowSize)(IDeviceContext*);
   double(*time)(IDeviceContext*);
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
double iDeviceContextTime(IDeviceContext *self);
void iDeviceContextDestroy(IDeviceContext *self);