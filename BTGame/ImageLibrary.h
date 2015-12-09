#pragma once

#include "segashared\Strings.h"

typedef struct Image_t Image;

typedef struct ImageLibrary_t ImageLibrary;

typedef struct ManagedImage_t ManagedImage;

void managedImageDestroy(ManagedImage *self);
Image *managedImageGetImage(ManagedImage *self);

ImageLibrary *imageLibraryCreate();
void imageLibraryDestroy(ImageLibrary *self);
ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView name);
int imageLibraryRegisterName(ImageLibrary *self, StringView name, const char *assetPath);

#define IMG_TILE_ATLAS "tile-atlas"
#define IMG_BG "bg"
#define IMG_BG_EDITOR "bg-editor"
#define IMG_BG_CONSOLE "bg-console"
#define IMG_VERBS "verbs"
#define IMG_CURSOR "cursor"
#define IMG_FONT "font"