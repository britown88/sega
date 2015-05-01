#pragma once

typedef unsigned char byte;
typedef struct BitBuffer_t BitBuffer;

#define EGA_COLORS 64
#define EGA_COLOR_UNDEFINED (EGA_COLORS)
#define EGA_COLOR_UNUSED (EGA_COLORS + 1)

#define EGA_PALETTE_COLORS 16
#define EGA_RES_WIDTH 640
#define EGA_RES_HEIGHT 350
#define EGA_TEXT_CHAR_WIDTH 8
#define EGA_TEXT_CHAR_HEIGHT 14
#define EGA_TEXT_RES_WIDTH (EGA_RES_WIDTH / EGA_TEXT_CHAR_WIDTH)
#define EGA_TEXT_RES_HEIGHT (EGA_RES_HEIGHT / EGA_TEXT_CHAR_HEIGHT)
#define EGA_PIXELS (EGA_RES_WIDTH * EGA_RES_HEIGHT)
#define EGA_BYTES (EGA_PIXELS / 8)
#define EGA_PIXEL_HEIGHT 1.37f
#define EGA_PIXEL_WIDTH 1.00f
#define EGA_PLANES 4
#define EGA_IMAGE_PLANES (EGA_PLANES + 1)

#define MAX_IMAGE_WIDTH 1024
#define MAX_IMAGE_HEIGHT 768

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
   SuperScanLine lines[MAX_IMAGE_HEIGHT];
} SuperBitPlane;

typedef struct Frame_t{
	BitPlane planes[EGA_PLANES];
} Frame;

Palette paletteDeserialize(const char *path);
void paletteSerialize(byte *data, const char *path);
Palette paletteCreatePartial(byte *data, byte pOffset, byte pCount, byte totalCount);

void paletteCopy(Palette *dest, Palette *src);

typedef struct Image_t Image;
typedef struct FlatImage_t FlatImage;

Frame *frameCreate();
void frameDestroy(Frame *self);
void frameRenderImage(Frame *self, short x, short y, Image *img);
void frameRenderPoint(Frame *self, short x, short y, byte color);
void frameRenderLine(Frame *self, short x1, short y1, short x2, short y2, byte color);
void frameClear(Frame *self, byte color);

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

FlatImage *imageRenderToFlat(Image *self);
short flatImageGetWidth(FlatImage *self);
short flatImageGetHeight(FlatImage *self);
SuperBitPlane *flatImageGetPlane(FlatImage *self, byte plane);
void flatImageDestroy(FlatImage *self);

//deserializes an ega file and maintains the file's scanline compression in memory
Image *imageDeserialize(const char*path);

//expands deserialized image into uncompressed scanlines which takes up mroe memory but renders faster
//also precomputes alpha
Image *imageDeserializeOptimized(const char*path);

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
PNGData *pngDataCreateFromImage(Image *img, byte *palette);
void pngDataDestroy(PNGData *self);

short pngDataGetWidth(PNGData *self);
short pngDataGetHeight(PNGData *self);

void pngDataRender(PNGData *self, byte *palette);

byte *pngDataGetPalette(PNGData *self);
Image *pngDataCreateImage(PNGData *self);
void pngDataExportPNG(PNGData *self, const char*path);

/*Text Rendering*/
typedef struct FontFactory_t FontFactory;
typedef struct Font_t Font;

/*
Image must be:
   - 256x112 with 256 8x14 characters organized according to ascii
   - solid 1 alpha (no transparency)
   - 2-color palette; 0 or background and 1 for foreground
*/
FontFactory *fontFactoryCreate(Image *fontImage);
void fontFactoryDestroy(FontFactory *self);

Font *fontFactoryGetFont(FontFactory *self, byte backGroundColor, byte foregroundColor);

void frameRenderText(Frame *frame, const char *text, short x, short y, Font *font);



#ifdef __cplusplus
};
#endif


