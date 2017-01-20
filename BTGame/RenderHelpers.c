
#include "RenderHelpers.h"

#include "segalib/EGA.h"
#include "ImageLibrary.h"
#include "WorldView.h"
#include "RichText.h"

FontFactory *initFontFactory(WorldView *view) {
   ManagedImage *fontImg = imageLibraryGetImage(view->imageLibrary, stringIntern(IMG_FONT));

   if (fontImg) {
      FontFactory *out = fontFactoryCreate(managedImageGetTexture(fontImg));
      managedImageDestroy(fontImg);
      return out;
   }

   return NULL;
}

void textureRenderSpan(WorldView *view, Texture *tex, byte *x, byte *y, Span *span) {
   Font *font;
   byte bg = 0, fg = 15;//default colors
   bool noSpace = span->style.flags&Style_NoSpace;

   if (span->style.flags&Style_Color) {
      bg = span->style.bg;
      fg = span->style.fg;
   }

   if (span->style.flags&Style_Invert) {
      bg = bg^fg;
      fg = bg^fg;
      bg = bg^fg;
   }

   font = fontFactoryGetFont(view->fontFactory, bg, fg);

   if (span->style.flags&Style_NoSpace) {
      textureRenderTextWithoutSpaces(tex, c_str(span->string), *x, *y, font);
   }
   else {
      textureRenderText(tex, c_str(span->string), *x, *y, font);
   }

   
   *x += (byte)stringLen(span->string);
}