
#include "RenderHelpers.h"

#include "segalib/EGA.h"
#include "ImageLibrary.h"
#include "WorldView.h"
#include "RichText.h"

FontFactory *initFontFactory(WorldView *view) {
   ManagedImage *fontImg = imageLibraryGetImage(view->imageLibrary, stringIntern(IMG_FONT));

   if (fontImg) {
      FontFactory *out = fontFactoryCreate(managedImageGetImage(fontImg));
      managedImageDestroy(fontImg);
      return out;
   }

   return NULL;
}

void frameRenderSpan(WorldView *view, Frame *frame, byte *x, byte *y, Span *span) {
   Font *font;
   byte bg = 0, fg = 15;//default colors

   if (span->style.flags&Style_Color) {
      bg = span->style.bg;
      fg = span->style.fg;
   }

   if (span->style.flags&Style_Invert) {
      bg |= (fg << 4);
      fg = bg & 15;
      bg >>= 4;
   }

   font = fontFactoryGetFont(view->fontFactory, bg, fg);

   frameRenderText(frame, c_str(span->string), *x, *y, font);
   *x += (byte)stringLen(span->string);
}