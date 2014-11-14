#include "EGA.h"
#include "EGAImage.hpp"
#include "bt-utils\CheckedMemory.h"

#include <malloc.h>

#include <memory>

struct PNGData_t {
   std::unique_ptr<EGAImage> image;
};

PNGData *pngDataCreate(const char *path) {
   PNGData *r = (PNGData *)checkedCalloc(1, sizeof(PNGData));
   r->image = std::unique_ptr<EGAImage>(new EGAImage(path));
   return r;
}

PNGData *pngDataCreateFromImage(Image *img, byte *palette) {
   PNGData *r = (PNGData *)checkedCalloc(1, sizeof(PNGData));
   r->image = std::unique_ptr<EGAImage>(new EGAImage(img, palette));
   return r;
}

void pngDataDestroy(PNGData *self) {
   self->image.reset();
   checkedFree(self);
}

short pngDataGetWidth(PNGData *self) {
   return self->image->width();
}
short pngDataGetHeight(PNGData *self) {
   return self->image->height();
}

void pngDataRender(PNGData *self, byte *palette) {
   self->image->renderWithPalette(palette);
}

byte *pngDataGetPalette(PNGData *self) {
   return self->image->palette();
}

Image *pngDataCreateImage(PNGData *self) {
   return self->image->toImage();
}

void pngDataExportPNG(PNGData *self, const char*path) {
   self->image->exportPNG(path);
}