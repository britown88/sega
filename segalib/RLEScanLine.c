#include "EGA.h"

#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include "CheckedMemory.h"
#include "BitBuffer.h"
#include "BitTwiddling.h"

typedef struct {
   ImageScanLine vsl;

   short width;
   short bitCount;
   byte *data;

} RLEScanLine;

static void _Destroy(RLEScanLine  *);
static void _Render(RLEScanLine  *, byte*);
static short _GetBitCount(RLEScanLine  *);
static void _Serialize(RLEScanLine  *, BitBuffer*);

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

ImageScanLine *createRLEScanLine(short bitCount, byte *data) {
   static byte rleBuffer[MAX_IMAGE_WIDTH];
   int compressed = compressBitsRLE(data, bitCount, rleBuffer);

   if(compressed > 0) {
      RLEScanLine *r = checkedCalloc(1, sizeof(RLEScanLine));

      r->vsl.vTable = _GetTable();

      r->bitCount = compressed;
      r->width = minByteCount(r->bitCount);
      r->data = checkedCalloc(1, r->width);

      memcpy(r->data, rleBuffer, r->width);

      return (ImageScanLine *)r;
   }

   return NULL;
}

ImageScanLine *createRLEScanLineFromBB(BitBuffer *buffer) {
   RLEScanLine *r = checkedCalloc(1, sizeof(RLEScanLine));

   r->vsl.vTable = _GetTable();

   r->bitCount = bitBufferReadShort(buffer);

   r->width = minByteCount(r->bitCount);
   r->data = checkedCalloc(1, r->width);

   bitBufferReadBits(buffer, r->data, r->bitCount);

   return (ImageScanLine *)r;
}

void _Destroy(RLEScanLine *self) {

   checkedFree(self->data);
   checkedFree(self);
}
void _Render(RLEScanLine  *self, byte *output) {

   decompressRLE(self->data, self->bitCount, output);
   //memcpy(output, self->data, self->width);
}
short _GetBitCount(RLEScanLine *self) {
   return self->bitCount;
}
void _Serialize(RLEScanLine *self, BitBuffer *destination) {
   byte fuckindumb = scanline_RLE;
   bitBufferWriteBits(destination, 2, &fuckindumb);//type ID
   bitBufferWriteBits(destination, sizeof(uint16_t)*8, (byte*)&self->bitCount);//bit length
   bitBufferWriteBits(destination, self->bitCount, self->data);//data
}