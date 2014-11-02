#include "EGA.h"

#include <malloc.h>
#include <string.h>
#include "CheckedMemory.h"
#include "BitBuffer.h"
#include "BitTwiddling.h"
#include <stdint.h>

typedef struct {
   ImageScanLine vsl;

   short width;
   short bitCount;
   byte *data;

   

} NormalFuckingScanLine;

static void _nfslDestroy(NormalFuckingScanLine  *);
static void _nfslRender(NormalFuckingScanLine  *, byte*);
static short _nfslGetBitCount(NormalFuckingScanLine  *);
static void _nfslSerialize(NormalFuckingScanLine  *, BitBuffer*);

static ImageScanLineVTable *nfslGetTable() {
   static ImageScanLineVTable *r = 0;
   if(!r) {
      r = calloc(1, sizeof(ImageScanLineVTable));
      r->destroy = (void (*)(ImageScanLine*))&_nfslDestroy;
      r->render = (void (*)(ImageScanLine*, byte*))&_nfslRender;
      r->getBitCount = (short (*)(ImageScanLine*))&_nfslGetBitCount;
      r->serialize = (void (*)(ImageScanLine*, BitBuffer*))&_nfslSerialize;
   }

   return r;
}

ImageScanLine *createUncompressedScanLine(short bitCount, byte *data){
   NormalFuckingScanLine *r = checkedCalloc(1, sizeof(NormalFuckingScanLine));

   r->vsl.vTable = nfslGetTable();

   r->bitCount = bitCount;
   r->width = minByteCount(r->bitCount);
   r->data = checkedCalloc(1, r->width);

   memcpy(r->data, data, r->width);


   return (ImageScanLine *)r;

}

ImageScanLine *createUncompressedScanLineFromBB(BitBuffer *buffer) {
   NormalFuckingScanLine *r = checkedCalloc(1, sizeof(NormalFuckingScanLine));

   r->vsl.vTable = nfslGetTable();

   r->bitCount = bitBufferReadShort(buffer);

   r->width = minByteCount(r->bitCount);
   r->data = checkedCalloc(1, r->width);

   bitBufferReadBits(buffer, r->data, r->bitCount);

   return (ImageScanLine *)r;
}

void _nfslDestroy(NormalFuckingScanLine *self) {

   checkedFree(self->data);
   checkedFree(self);
}
void _nfslRender(NormalFuckingScanLine  *self, byte *output) {
   memcpy(output, self->data, self->width);
}
short _nfslGetBitCount(NormalFuckingScanLine *self) {
   return self->bitCount;
}
void _nfslSerialize(NormalFuckingScanLine *self, BitBuffer *destination) {
   byte fuckindumb = scanline_UNCOMPRESSED;
   bitBufferWriteBits(destination, 2, &fuckindumb);//type ID
   bitBufferWriteBits(destination, sizeof(uint16_t)*8, (byte*)&self->bitCount);//bit length
   bitBufferWriteBits(destination, self->bitCount, self->data);//data
}