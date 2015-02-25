#pragma once

#include "segashared\Strings.h"

typedef struct Image_t Image;

typedef struct ImageManager_t ImageManager;

typedef struct ManagedImage_t ManagedImage;

void managedImageDestroy(ManagedImage *self);
Image *managedImageGetImage(ManagedImage *self);

ImageManager *imageManagerCreate();
void imageManagerDestroy(ImageManager *self);
ManagedImage *imageManagerGetImage(ImageManager *self, StringView path);