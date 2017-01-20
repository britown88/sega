#pragma once

#include "segautils/Vector.h"

typedef struct MenuPanel_t MenuPanel;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FrameRegion_t FrameRegion;
typedef struct Texture_t Texture;

MenuPanel *menuPanelCreate(ImageLibrary *library, short x, short y, short width, short height);
void menuPanelDestroy(MenuPanel *self);

void menuPanelRender(MenuPanel *self, Texture *tex, FrameRegion *region);

void menuPanelSetPosition(MenuPanel *self, Int2 pos);
void menuPanelSetSize(MenuPanel *self, Int2 size);