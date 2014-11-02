/************************************
   File: main.c
   Author: Brandon Delorius Townsend
   Date: 20XDX
   FUnction: It's fucking main, bro

   COPYRIGHT TODAY
************************************/
#include "App.h"
#include "SegaApp.h"
#include "segalib\CheckedMemory.h"

int main()
{
   runApp(SegaAppCreate());
   printMemoryLeaks();

   return 0;
}