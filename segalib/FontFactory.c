#include "EGA.h"
#include "segashared\CheckedMemory.h"
#include "segautils\BitTwiddling.h"
#include <string.h>

#define FONT_CHAR_WIDTH 32
#define FONT_CHAR_HEIGHT 8

#define FONT_FILE_WIDTH (FONT_CHAR_WIDTH * EGA_TEXT_CHAR_WIDTH)
#define FONT_FILE_HEIGHT (FONT_CHAR_HEIGHT * EGA_TEXT_CHAR_HEIGHT)



typedef struct {
   byte pixels[FONT_FILE_WIDTH/8];
} FontScanLine;

typedef struct {
	FontScanLine lines[FONT_FILE_HEIGHT];
} FontBitPlane;

struct Font_t{
   byte built;
	FontBitPlane *planes;
};

struct FontFactory_t {
   FontBitPlane textPlane;
   Font fonts[256];
};


FontFactory *fontFactoryCreate(Image *fontImage) {
   FontFactory *r = 0;
   int y;

   if(imageGetWidth(fontImage) < FONT_FILE_WIDTH ||
      imageGetHeight(fontImage) < FONT_FILE_HEIGHT) {
         return 0;
   }
      
   r = checkedCalloc(1, sizeof(FontFactory));

   for(y = 0; y < FONT_FILE_HEIGHT; ++y) {
      imageScanLineRender(imageGetScanLine(fontImage, y, 1), r->textPlane.lines[y].pixels);      
   }

   return r;
}

void fontFactoryDestroy(FontFactory *self) {
   int i;
   for(i = 0; i < 256; ++i){
      if(self->fonts[i].built){
         checkedFree(self->fonts[i].planes);
      }
   }

   checkedFree(self);
}

Font *fontFactoryGetFont(FontFactory *self, byte backgroundColor, byte foregroundColor){
   byte index = backgroundColor + (foregroundColor <<4);
   if(!self->fonts[index].built) {

      //build the font
      int x, y, i;

      self->fonts[index].planes = checkedCalloc(EGA_PLANES, sizeof(FontBitPlane));

      for(y = 0; y < FONT_FILE_HEIGHT; ++y){
         for(x = 0; x < FONT_FILE_WIDTH; ++x) {
            byte textVal = getBitFromArray(self->textPlane.lines[y].pixels, x);
            byte color = textVal == 0 ? backgroundColor : foregroundColor;

            for(i = 0; i < EGA_PLANES; ++i) {
               byte *line = self->fonts[index].planes[i].lines[y].pixels;
               setBitInArray(line, x, getBit(color, i));
            }
         }
      }

      self->fonts[index].built = 1;
   }

   return &self->fonts[index];
}

void frameRenderText(Frame *frame, const char *text, short x, short y, Font *font){
   size_t charCount;
   size_t c;
   int i, iy;

   if (!text){
      return;
   }

   charCount = strlen(text);

   for(c = 0; c < charCount; ++c) {
      byte uc = *(unsigned char*)&text[c];
      byte charY = uc / FONT_CHAR_WIDTH;
      byte charX = uc % FONT_CHAR_WIDTH;

      if(x >= EGA_TEXT_RES_WIDTH)
         break;

      for(iy = 0; iy < EGA_TEXT_CHAR_HEIGHT; ++iy) {
         for(i = 0; i < EGA_PLANES; ++i) {
            short linePos = y * EGA_TEXT_CHAR_HEIGHT + iy;
            short charLinePos = charY * EGA_TEXT_CHAR_HEIGHT + iy;

            frame->planes[i].lines[linePos].pixels[x] = font->planes[i].lines[charLinePos].pixels[charX];
         }
      }

      ++x;
   }

}