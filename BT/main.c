/************************************
   File: main.c
   Author: Brandon Delorius Townsend
   Date: 20XDX
   FUnction: It's fucking main, bro

   COPYRIGHT TODAY
************************************/
#include "SEGA\App.h"
#include "BT.h"
#include "segalib\CheckedMemory.h"

int main()
{
   runApp((VirtualApp*)btCreate());
   printMemoryLeaks();

   return 0;
}