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
#include "GLFWContext\GLFWContext.h"

int main()
{
   runApp(btCreate(), createOGLRenderer(), createGLFWContext());
   printMemoryLeaks();

   return 0;
}