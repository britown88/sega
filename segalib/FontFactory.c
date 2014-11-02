#include "EGA.h"
#include "CheckedMemory.h"
#include "BitTwiddling.h"
#include <string.h>

#define FONT_FILE_WIDTH 256
#define FONT_FILE_HEIGHT 112

#define FONT_CHAR_WIDTH (FONT_FILE_WIDTH/ EGA_TEXT_CHAR_WIDTH)
#define FONT_CHAR_HEIGHT (FONT_FILE_HEIGHT / EGA_TEXT_CHAR_HEIGHT)



typedef struct {
   byte pixels[FONT_FILE_WIDTH/8];
} FontScanLine;

typedef struct {
	FontScanLine lines[FONT_FILE_HEIGHT];
} FontBitPlane;

typedef struct Font_t{
   byte built;
	FontBitPlane planes[EGA_PLANES];
};

typedef struct FontFactory_t {
   FontBitPlane textPlane;
   Font fonts[256];
};


FontFactory *fontFactoryCreate(Image *fontImage) {
   FontFactory *r = 0;
   int y;

   if(imageGetWidth(fontImage) != FONT_FILE_WIDTH ||
      imageGetHeight(fontImage) != FONT_FILE_HEIGHT) {
         return 0;
   }
      
   r = checkedCalloc(1, sizeof(FontFactory));

   for(y = 0; y < FONT_FILE_HEIGHT; ++y) {
      imageScanLineRender(imageGetScanLine(fontImage, y, 1), r->textPlane.lines[y].pixels);      
   }

   return r;
}

void fontFactoryDestroy(FontFactory *self) {
   checkedFree(self);
}

Font *fontFactoryGetFont(FontFactory *self, byte backgroundColor, byte foregroundColor){
   byte index = backgroundColor + (foregroundColor <<4);
   if(!self->fonts[index].built) {

      //build the font
      int x, y, i;

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
   size_t charCount = strlen(text);
   int c, i, iy;

   for(c = 0; c < charCount; ++c) {
      byte charY = text[c] / FONT_CHAR_WIDTH;
      byte charX = text[c] % FONT_CHAR_WIDTH;

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