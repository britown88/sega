/************************************
   File: main.c
   Author: Brandon Delorius Townsend
   Date: 20XDX
   FUnction: It's fucking main, bro

   COPYRIGHT TODAY
************************************/
#include "SEGA\App.h"
#include "BT.h"
#include "segashared\CheckedMemory.h"
#include "GLSLRenderer\GLSLRenderer.h"

int main()
{
   runApp(btCreate(), createGLSLRenderer());
   printMemoryLeaks();

   return 0;
}