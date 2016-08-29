#pragma once

#include "segautils/Vector.h"

typedef struct MenuPanel_t MenuPanel;
typedef struct ImageLibrary_t ImageLibrary;
typedef struct FrameRegion_t FrameRegion;
typedef struct Frame_t Frame;

MenuPanel *menuPanelCreate(ImageLibrary *library, short x, short y, short width, short height);
void menuPanelDestroy(MenuPanel *self);

void menuPanelRender(MenuPanel *self, Frame *f, FrameRegion *region);

void menuPanelSetPosition(MenuPanel *self, Int2 pos);
void menuPanelSetSize(MenuPanel *self, Int2 size);