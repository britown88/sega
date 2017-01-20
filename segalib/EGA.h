#pragma once

#include "config.h"

typedef unsigned char byte;
typedef struct BitBuffer_t BitBuffer;

#if defined(EGA_MODE_0Dh)
#define EGA_RES_WIDTH 320
#define EGA_RES_HEIGHT 200
#define EGA_TEXT_CHAR_WIDTH 8
#define EGA_TEXT_CHAR_HEIGHT 8
#define EGA_PIXEL_HEIGHT 1.2f
#define EGA_PIXEL_WIDTH 1.00f
#elif defined(EGA_MODE_10h)
#define EGA_RES_WIDTH 640
#define EGA_RES_HEIGHT 350
#define EGA_TEXT_CHAR_WIDTH 8
#define EGA_TEXT_CHAR_HEIGHT 14
#define EGA_PIXEL_HEIGHT 1.37f
#define EGA_PIXEL_WIDTH 1.00f
#else
#error segalib: You must define a video mode in config.h
#endif

#define EGA_COLORS 64
#define EGA_COLOR_UNDEFINED (EGA_COLORS)
#define EGA_COLOR_UNUSED (EGA_COLORS + 1)
#define EGA_PALETTE_COLORS 16
#define EGA_TEXT_RES_WIDTH (EGA_RES_WIDTH / EGA_TEXT_CHAR_WIDTH)
#define EGA_TEXT_RES_HEIGHT (EGA_RES_HEIGHT / EGA_TEXT_CHAR_HEIGHT)
#define EGA_PIXELS (EGA_RES_WIDTH * EGA_RES_HEIGHT)
#define EGA_BYTES (EGA_PIXELS / 8)
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

typedef struct FrameRegion_t{
   int origin_x, origin_y, width, height;
}FrameRegion;
static FrameRegion FrameRegionFULL_CONCRETE = { 0, 0, EGA_RES_WIDTH, EGA_RES_HEIGHT };
static FrameRegion *FrameRegionFULL = &FrameRegionFULL_CONCRETE;

typedef struct Frame_t{
	BitPlane planes[EGA_PLANES];
} Frame;

Palette paletteDeserialize(const char *path);
void paletteSerialize(byte *data, const char *path);
Palette paletteCreatePartial(byte *data, byte pOffset, byte pCount, byte totalCount);

void paletteCopy(Palette *dest, Palette *src);

typedef struct Image_t Image;

Frame *frameCreate();
void frameDestroy(Frame *self);
void frameRenderImage(Frame *self, FrameRegion *vp, short x, short y, Image *img);
void frameRenderImagePartial(Frame *self, FrameRegion *vp, short x, short y, Image *img, short imgX, short imgY, short imgWidth, short imgHeight);
void frameRenderPoint(Frame *self, FrameRegion *vp, short x, short y, byte color);
void frameRenderLine(Frame *self, FrameRegion *vp, short x1, short y1, short x2, short y2, byte color);
void frameRenderLineRect(Frame *self, FrameRegion *vp, short left, short top, short right, short bottom, byte color);
void frameRenderRect(Frame *self, FrameRegion *vp, short left, short top, short right, short bottom, byte color);
void frameClear(Frame *self, FrameRegion *vp, byte color);

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

typedef struct FlatImage_t{
   short width, height;
   SuperBitPlane planes[EGA_IMAGE_PLANES];
}FlatImage;

void imageRenderToFlat(Image *self, FlatImage *dest);

//deserializes an ega file and maintains the file's scanline compression in memory
#define EGA_IMGD_FILEPATH (1 << 0)//input buffer is assumed to be a null terminated string to a filepath
#define EGA_IMGD_OPTIMIZED (1 << 1)//output is deserialized into uncompressed scanlines which takes up mroe memory but renders faster
#define EGA_IMGD_OWN (1 << 2) //function takes ownership of the input buffer and frees it when done

#define EGA_IMGD_LEGACY (EGA_IMGD_FILEPATH|EGA_IMGD_OPTIMIZED|EGA_IMGD_OWN)

//buffer should be a nullterminated string if flag has FILEPATH or a byte* otherwise
Image *imageDeserialize(const void *buffer, int flags);

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
void frameRenderTextWithoutSpaces(Frame *frame, const char *text, short x, short y, Font *font);

/*New texture system 2017
Textures are flat byte*'s with height, width, and a format specifier
Going to move to a system wher einstead of Image* and Texture* there is just Image Textures (with alpha channel) and frame textures
This way we can standardize all drawing functions to allow drawing to arbitrary textures and get FBO functionality
Image* will be reduced to only being a serialization layer and frame will be reduced to a renderTexture function
*/
typedef struct Texture_t Texture;


Texture *textureCreate(int width, int height);
Texture *imageCreateTexture(Image *self);
void textureDestroy(Texture *self);

int textureGetWidth(Texture *self);
int textureGetHeight(Texture *self);

//raw data! ten cuidado!
byte *textureGetScanline(Texture *self, byte plane, int y);
byte *textureGetAlphaScanline(Texture *self, int y);

/*
FrameRegion NOTE: Passs NULL as a frameregion to use the full image
*/

void textureClear(Texture *self, FrameRegion *vp, byte color);
void textureClearAlpha(Texture *self);
void textureRenderTexture(Texture *self, FrameRegion *vp, int x, int y, Texture *tex);
void textureRenderTexturePartial(Texture *self, FrameRegion *vp, int x, int y, Texture *tex, int texX, int texY, int texWidth, int texHeight);
void textureRenderPoint(Texture *self, FrameRegion *vp, int x, int y, byte color);
void textureRenderLine(Texture *self, FrameRegion *vp, int x1, int y1, int x2, int y2, byte color);
void textureRenderLineRect(Texture *self, FrameRegion *vp, int left, int top, int right, int bottom, byte color);
void textureRenderRect(Texture *self, FrameRegion *vp, int left, int top, int right, int bottom, byte color);
void textureRenderText(Texture *texture, const char *text, int x, int y, Font *font);
void textureRenderTextWithoutSpaces(Texture *texture, const char *text, int x, int y, Font *font);

void frameRenderTexture(Frame *self, FrameRegion *vp, short x, short y, Texture *tex);


#ifdef __cplusplus
};
#endif


