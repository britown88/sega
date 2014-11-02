#pragma once

typedef unsigned char byte;
typedef struct BitBuffer_t BitBuffer;

#define EGA_COLORS 64
#define EGA_PALETTE_COLORS 16
#define EGA_RES_WIDTH 640
#define EGA_RES_HEIGHT 350
#define EGA_PIXELS (EGA_RES_WIDTH * EGA_RES_HEIGHT)
#define EGA_BYTES (EGA_PIXELS / 8)
#define EGA_PIXEL_HEIGHT 1.37f
#define EGA_PIXEL_WIDTH 1.00f
#define EGA_PLANES 4
#define EGA_IMAGE_PLANES (EGA_PLANES + 1)

#define MAX_IMAGE_WIDTH 1024

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	byte colors[EGA_PALETTE_COLORS];
} Palette;

typedef struct {
	byte pixels[EGA_RES_WIDTH/8];
} ScanLine;

typedef struct {
   byte pixels[MAX_IMAGE_WIDTH];
} SuperScanLine;

typedef struct {
	ScanLine lines[EGA_RES_HEIGHT];
} BitPlane;

typedef struct {
	BitPlane planes[EGA_PLANES];
} Frame;

Palette paletteDeserialize(const char *path);
void paletteSerialize(byte *data, const char *path);

typedef struct Image_t Image;

Frame *frameCreate();
void frameDestroy(Frame *self);
void frameRenderImage(Frame *self, short x, short y, Image *img);

void scanLineSetBit(ScanLine *self, short position, byte value);
byte scanLineGetBit(ScanLine *self, short position);

typedef struct ImageScanLine_t ImageScanLine;

typedef struct 
{
   void (*destroy)(ImageScanLine*);
   void (*render)(ImageScanLine*, byte*);
   short (*getBitCount)(ImageScanLine*);
   void (*serialize)(ImageScanLine*, BitBuffer *);
} ImageScanLineVTable;

struct ImageScanLine_t {
    ImageScanLineVTable* vTable;
};

void imageScanLineDestroy(ImageScanLine *self);
void imageScanLineRender(ImageScanLine *self, byte *output);
short imageScanLineGetBitCount(ImageScanLine *self);
void imageScanLineSerialize(ImageScanLine *self, BitBuffer *dest);

typedef struct Image_t Image;

Image *imageCreate(short width, short height);
Image *imageDeserialize(const char*path);
void imageSerialize(Image *self, const char *path);
void imageDestroy(Image *self);

void imageSetScanLine(Image *self, short position, byte plane, ImageScanLine *sl);
ImageScanLine *imageGetScanLine(Image *self, short position, byte plane);

short imageGetWidth(Image *self);
short imageGetHeight(Image *self);

enum {
   scanline_SOLID = 0,
   scanline_UNCOMPRESSED = 1,
   scanline_RLE = 2
};

ImageScanLine *createSmallestScanLine(short bitCount, byte *data);

ImageScanLine *createUncompressedScanLine(short bitCount, byte *data);
ImageScanLine *createRLEScanLine(short bitCount, byte *data);
ImageScanLine *createSolidScanLine(short bitCount, byte *data);

ImageScanLine *createUncompressedScanLineFromBB(BitBuffer *buffer);
ImageScanLine *createRLEScanLineFromBB(BitBuffer *buffer);
ImageScanLine *createSolidScanLineFromBB(BitBuffer *buffer, short imgWidth);


Frame *buildCheckerboardFrame(int width, byte color1, byte color2);
Image *buildCheckerboardImage(int width, int height, int tileWidth, byte color1, byte color2);

typedef struct PNGData_t PNGData;

int getEGAColor(byte c);

PNGData *pngDataCreate(const char *path);
void pngDataDestroy(PNGData *self);

short pngDataGetWidth(PNGData *self);
short pngDataGetHeight(PNGData *self);

void pngDataRender(PNGData *self, byte *palette, byte pOffset, byte pColorCount, byte totalColorCount);

byte *pngDataGetPalette(PNGData *self);
Image *pngDataCreateImage(PNGData *self);
void pngDataExportPNG(PNGData *self, const char*path);


#ifdef __cplusplus
};
#endif


