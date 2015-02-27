#pragma once

#include "segautils\Vector.h"
#include "segashared\Strings.h"
#include "segautils\DLLBullshit.h"

typedef struct GLWindow_T GLWindow;
typedef struct GLFWmonitor GLFWmonitor;

GLWindow *glWindowCreate(Int2 winSize, StringView windowName, GLFWmonitor *monitor);
void glWindowDestroy(GLWindow *self);

void glWindowPollEvents(GLWindow *self);
void glWindowSwapBuffers(GLWindow *self);
int glWindowShouldClose(GLWindow *self);

Int2 glWindowGetSize(GLWindow *self);


typedef struct IDeviceContext_t IDeviceContext;

DLL_PUBLIC IDeviceContext *testGLContext();