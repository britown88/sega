#include "EGA.h"
#include "segashared\CheckedMemory.h"
#include "segautils\BitTwiddling.h"
#include <string.h>
#include "segautils/Defs.h"

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


FontFactory *fontFactoryCreate(Texture *fontTexture) {
   FontFactory *r = 0;
   int y;

   if(textureGetWidth(fontTexture) < FONT_FILE_WIDTH ||
      textureGetHeight(fontTexture) < FONT_FILE_HEIGHT) {
         return 0;
   }
      
   r = checkedCalloc(1, sizeof(FontFactory));

   for(y = 0; y < FONT_FILE_HEIGHT; ++y) {
      memcpy(r->textPlane.lines[y].pixels, textureGetScanline(fontTexture, 0, y), sizeof(FontScanLine));     
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

void frameRenderTextSingleChar(Frame *frame, const char c, short x, short y, Font *font, bool spaces) {
   int i, iy;
   byte charY = c / FONT_CHAR_WIDTH;
   byte charX = c % FONT_CHAR_WIDTH;

   if (x >= EGA_TEXT_RES_WIDTH || (!spaces && c == ' ')) {
      return;
   }

   for (iy = 0; iy < EGA_TEXT_CHAR_HEIGHT; ++iy) {
      for (i = 0; i < EGA_PLANES; ++i) {
         short linePos = y * EGA_TEXT_CHAR_HEIGHT + iy;
         short charLinePos = charY * EGA_TEXT_CHAR_HEIGHT + iy;
         frame->planes[i].lines[linePos].pixels[x] = font->planes[i].lines[charLinePos].pixels[charX];
      }
   }
}

static void _frameRenderTextEX(Frame *frame, const char *text, short x, short y, Font *font, bool drawSpaces){
   size_t charCount;
   size_t c;

   if (!text){
      return;
   }

   charCount = strlen(text);

   for(c = 0; c < charCount; ++c) {
      if(x >= EGA_TEXT_RES_WIDTH)
         break;

      frameRenderTextSingleChar(frame, *(unsigned char*)&text[c], x++, y, font, drawSpaces);
   }
}

void frameRenderText(Frame *frame, const char *text, short x, short y, Font *font) {
   _frameRenderTextEX(frame, text, x, y, font, true);
}

void frameRenderTextWithoutSpaces(Frame *frame, const char *text, short x, short y, Font *font) {
   _frameRenderTextEX(frame, text, x, y, font, false);
}

void textureRenderTextSingleChar(Texture *tex, const char c, int x, int y, Font *font, int spaces) {
   int yIter, plane;
   byte charY = c / FONT_CHAR_WIDTH;
   byte charX = c % FONT_CHAR_WIDTH;

   if (!spaces && c == ' ') {
      return;
   }

   for (yIter = 0; yIter < EGA_TEXT_CHAR_HEIGHT; ++yIter) {
      short linePos = y * EGA_TEXT_CHAR_HEIGHT + yIter;
      short charLinePos = charY * EGA_TEXT_CHAR_HEIGHT + yIter;

      for (plane = 0; plane < EGA_PLANES; ++plane) {
         *(textureGetScanline(tex, plane, linePos) + x) = font->planes[plane].lines[charLinePos].pixels[charX];
      }

      *(textureGetAlphaScanline(tex, linePos) + x) = 0;
   }
}

static void _textureRenderTextEX(Texture *tex, const char *text, int x, int y, Font *font, bool drawSpaces) {
   size_t charCount;
   size_t c;
   int texCharCount = textureGetWidth(tex) / EGA_TEXT_CHAR_WIDTH;

   if (!text) {
      return;
   }

   charCount = strlen(text);

   for (c = 0; c < charCount; ++c) {
      if (x >= texCharCount)
         break;

      textureRenderTextSingleChar(tex, *(unsigned char*)&text[c], x++, y, font, drawSpaces);
   }
}

void textureRenderText(Texture *tex, const char *text, int x, int y, Font *font) {
   _textureRenderTextEX(tex, text, x, y, font, true);
}

void textureRenderTextWithoutSpaces(Texture *tex, const char *text, int x, int y, Font *font) {
   _textureRenderTextEX(tex, text, x, y, font, false);
}