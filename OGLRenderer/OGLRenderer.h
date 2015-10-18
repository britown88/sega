#include "SEGA\IRenderer.h"
#include "segautils\DLLBullshit.h"
#include "SEGA/IDeviceContext.h"

DLL_PUBLIC IRenderer *createOGLRenderer(IDeviceContext *context);