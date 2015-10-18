/************************************
   File: main.c
   Author: Brandon Delorius Townsend
   Date: 20XDX
   FUnction: It's fucking main, bro

   COPYRIGHT TODAY
************************************/
#include "SEGA\App.h"
#include "ShiftGame\Shift.h"
#include "segashared\CheckedMemory.h"
#include "OGLRenderer\OGLRenderer.h"
#include "GLFWContext\GLFWContext.h"

int main()
{
   IDeviceContext *context = createGLFWContext();
   runApp(shiftCreate(), createOGLRenderer(context), context);
   printMemoryLeaks();

   return 0;
}