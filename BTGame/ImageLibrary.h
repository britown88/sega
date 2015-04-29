#pragma once

#include "segashared\Strings.h"

typedef struct Image_t Image;

typedef struct ImageLibrary_t ImageLibrary;

typedef struct ManagedImage_t ManagedImage;

void managedImageDestroy(ManagedImage *self);
Image *managedImageGetImage(ManagedImage *self);

ImageLibrary *imageLibraryCreate();
void imageLibraryDestroy(ImageLibrary *self);
ManagedImage *imageLibraryGetImage(ImageLibrary *self, StringView path);