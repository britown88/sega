#include "ColorMath.hpp"

/* Return gamma-corrected value of a color component byte*/
float GCRGB(byte component) {
   static float GCRGBTable[256] = {0.0f};
   static int loaded = 0;
 
   if(!loaded) {
      int i;
      for(i = 0; i < 256; ++i) {
         GCRGBTable[i] = pow(i/255.0f, 2.2f);
      }
      loaded = 1;
   }
 
   return GCRGBTable[component];
}
 
float colorDistance(Color c1, Color c2) {
   float r, g, b;
   r = GCRGB(c1.r) - GCRGB(c2.r);
   g = GCRGB(c1.g) - GCRGB(c2.g);
   b = GCRGB(c1.b) - GCRGB(c2.b);

   return r*r + g*g + b*b;
}


Color EGAColorLookup(byte c) {
   auto ci = getEGAColor(c);
   Color r = *(Color*)&ci;
   return r;
}

byte closestEGA(int rgb) {
   float lowest = 1000.0;
   int closest = 0;

   for(int i = 0; i < 64; ++i){
      auto &c = EGAColorLookup(i);

      float diff = colorDistance(*(Color*)&rgb, c);

      if(diff < lowest) {
         lowest = diff;
         closest = i;
      }
   }

   return closest;
}

void insertSortedPaletteEntry(ImageColor &color, PaletteColor &parent, byte target, byte current, PaletteEntry &out) {

   out.color = &parent;
   out.distance = sqrt(colorDistance(EGAColorLookup(target), EGAColorLookup(current)));

   auto iter = color.closestColor;
   if(!iter){
      out.next = out.prev = nullptr;
      color.closestColor = &out;
   }
   else {
      PaletteEntry *prev = nullptr;
      while(iter && out.distance > iter->distance) {
         prev = iter;
         iter = iter->next;
      }

      if(!prev) {
         out.prev = nullptr;
         out.next = color.closestColor;
         out.next->prev = &out;
         color.closestColor = &out;
      }
      else {
         out.next = iter;
         out.prev = prev;
         if(out.next) {
            out.next->prev = &out;            
         }

         out.prev->next = &out;
      }
   }
}

bool isClosest(ImageColor &color, PaletteEntry &entry) {
   return color.closestColor == &entry;
}