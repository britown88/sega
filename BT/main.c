/************************************
File: main.c
Author: Brandon Delorius Townsend
Date: 20XDX
FUnction: It's fucking main, bro

COPYRIGHT TODAY
************************************/
#include "SEGA\App.h"
#include "BTGame\BT.h"
#include "segashared\CheckedMemory.h"
#include "OGLRenderer\OGLRenderer.h"
#include "SDL2Context\SDLContext.h"

int main()
{
   IDeviceContext *context = createSDLContext();
   runApp(btCreate(), createOGLRenderer(context), context);
   printMemoryLeaks();

   return 0;
}