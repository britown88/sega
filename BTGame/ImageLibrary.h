#pragma once

#include "segashared\Strings.h"

typedef struct Image_t Image;

typedef struct ImageLibrary_t ImageLibrary;

typedef struct ManagedImage_t ManagedImage;
typedef struct Texture_t Texture;

typedef struct WorldView_t WorldView;

void managedImageDestroy(ManagedImage *self);
Texture *managedImageGetTexture(ManagedImage *self);

ImageLibrary *imageLibraryCreate(WorldView *view);
void imageLibraryDestroy(ImageLibrary *self);
ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView name);
void imageLibraryClear(ImageLibrary *self);

#define IMG_TILE_ATLAS "tiles"
#define IMG_BG "bg"
#define IMG_BG_EDITOR "bgeditor"
#define IMG_BG_CONSOLE "bgconsole"
#define IMG_VERBS "verbs"
#define IMG_CURSOR "cursor"

#include "segalib/EGA.h"

#ifdef EGA_MODE_10h
#define IMG_FONT "font814"
#else
#define IMG_FONT "font"
#endif