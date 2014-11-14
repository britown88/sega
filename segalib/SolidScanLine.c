#include "EGA.h"

#include <malloc.h>
#include <string.h>
#include "segashared\CheckedMemory.h"
#include "segautils\BitBuffer.h"
#include "segautils\BitTwiddling.h"

typedef struct {
   ImageScanLine vsl;

   short width;
   byte value;

} SolidScanLine;

static void _Destroy(SolidScanLine  *);
static void _Render(SolidScanLine  *, byte*);
static short _GetBitCount(SolidScanLine  *);
static void _Serialize(SolidScanLine  *, BitBuffer*);

static ImageScanLineVTable *_GetTable() {
   static ImageScanLineVTable *r = 0;
   if(!r) {
      r = calloc(1, sizeof(ImageScanLineVTable));
      r->destroy = (void (*)(ImageScanLine*))&_Destroy;
      r->render = (void (*)(ImageScanLine*, byte*))&_Render;
      r->getBitCount = (short (*)(ImageScanLine*))&_GetBitCount;
      r->serialize = (void (*)(ImageScanLine*, BitBuffer*))&_Serialize;
   }

   return r;
}

ImageScanLine *createSolidScanLine(short bitCount, byte *data) {

   if(arrayIsSolid(data, bitCount)){
      SolidScanLine *r = checkedCalloc(1, sizeof(SolidScanLine));

      r->vsl.vTable = _GetTable();

      r->width = bitCount;
      r->value = getBit(*data, 0);

      return (ImageScanLine *)r;
   }

   return NULL;
}

ImageScanLine *createSolidScanLineFromBB(BitBuffer *buffer, short imgWidth) {
   SolidScanLine *r = checkedCalloc(1, sizeof(SolidScanLine));

   r->vsl.vTable = _GetTable();
   r->width = imgWidth;
   bitBufferReadBits(buffer, &r->value, 1);


   return (ImageScanLine *)r;
}

void _Destroy(SolidScanLine *self) {
   checkedFree(self);
}
void _Render(SolidScanLine  *self, byte *output) {
   memset(output, self->value ? 255 : 0, minByteCount(self->width));
}
short _GetBitCount(SolidScanLine *self) {
   return 1;
}
void _Serialize(SolidScanLine *self, BitBuffer *destination) {
   byte fuckindumb = scanline_SOLID;
   bitBufferWriteBits(destination, 2, &fuckindumb);//type ID
   bitBufferWriteBits(destination, 1, &self->value);

}