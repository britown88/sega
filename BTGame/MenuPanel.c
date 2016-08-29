#include "MenuPanel.h"
#include "segashared/CheckedMemory.h"
#include "ImageLibrary.h"
#include "segalib/EGA.h"

#define BORDER_SIZE 8

struct MenuPanel_t {
   ManagedImage *borderImg;
   short x, y, width, height;
};

MenuPanel *menuPanelCreate(ImageLibrary *library, short x, short y, short width, short height) {
   MenuPanel *out = checkedCalloc(1, sizeof(MenuPanel));

   out->borderImg = imageLibraryGetImage(library, stringIntern("border"));
   out->x = x;
   out->y = y;
   out->width = width;
   out->height = height;

   return out;
}
void menuPanelDestroy(MenuPanel *self) {
   managedImageDestroy(self->borderImg);
   checkedFree(self);
}

void menuPanelSetPosition(MenuPanel *self, Int2 pos) {
   self->x = pos.x;
   self->y = pos.y;
}
void menuPanelSetSize(MenuPanel *self, Int2 size) {
   self->width = size.x;
   self->height = size.y;
}

typedef struct {
   MenuPanel *mp;
   Image *img;
   Frame *f;
   FrameRegion *r;
}MPRenderData;



static void _renderPieceEX(MPRenderData *data, short x, short y, short ix, short iy, short w, short h) {
   frameRenderImagePartial(data->f, data->r, x, y, data->img, ix * BORDER_SIZE, iy * BORDER_SIZE, w, h);
}

static void _renderPiece(MPRenderData *data, short x, short y, short ix, short iy) {
   _renderPieceEX(data, x, y, ix, iy, BORDER_SIZE, BORDER_SIZE);
}

static void _renderCorners(MPRenderData *data) {
   _renderPiece(data, data->mp->x - BORDER_SIZE, data->mp->y - BORDER_SIZE, 0, 0);
   _renderPiece(data, data->mp->x + data->mp->width, data->mp->y - BORDER_SIZE, 2, 0);
   _renderPiece(data, data->mp->x - BORDER_SIZE, data->mp->y + data->mp->height, 0, 2);
   _renderPiece(data, data->mp->x + data->mp->width, data->mp->y + data->mp->height, 2, 2);
}

static void _renderTop(MPRenderData *data) {
   int x = 0;
   while (x + BORDER_SIZE < data->mp->width) {
      _renderPiece(data, data->mp->x + x, data->mp->y - BORDER_SIZE, 1, 0);
      x += BORDER_SIZE ;
   }
   if (x < data->mp->width) {
      _renderPieceEX(data, data->mp->x + x, data->mp->y - BORDER_SIZE, 1, 0, data->mp->width - x, BORDER_SIZE);
   }
}

static void _renderLeft(MPRenderData *data) {
   int y = 0;
   while (y + BORDER_SIZE < data->mp->height) {
      _renderPiece(data, data->mp->x - BORDER_SIZE, data->mp->y + y, 0, 1);
      y += BORDER_SIZE;
   }
   if (y < data->mp->height) {
      _renderPieceEX(data, data->mp->x - BORDER_SIZE, data->mp->y + y, 0, 1, BORDER_SIZE, data->mp->height - y);
   }
}

static void _renderBottom(MPRenderData *data) {
   int x = 0;
   while (x + BORDER_SIZE < data->mp->width) {
      _renderPiece(data, data->mp->x + x, data->mp->y + data->mp->height, 1, 2);
      x += BORDER_SIZE;
   }
   if (x < data->mp->width) {
      _renderPieceEX(data, data->mp->x + x, data->mp->y + data->mp->height, 1, 2, data->mp->width - x, BORDER_SIZE);
   }
}

static void _renderRight(MPRenderData *data) {
   int y = 0;
   while (y + BORDER_SIZE < data->mp->height) {
      _renderPiece(data, data->mp->x + data->mp->width, data->mp->y + y, 2, 1);
      y += BORDER_SIZE;
   }
   if (y < data->mp->height) {
      _renderPieceEX(data, data->mp->x + data->mp->width, data->mp->y + y, 2, 1, BORDER_SIZE, data->mp->height - y);
   }
}



void menuPanelRender(MenuPanel *s, Frame *f, FrameRegion *r) {
   MPRenderData data = {s, managedImageGetImage(s->borderImg), f, r};

   _renderCorners(&data);
   _renderTop(&data);
   _renderBottom(&data);
   _renderLeft(&data);
   _renderRight(&data);

   frameRenderRect(f, r, s->x, s->y, s->x + s->width, s->y + s->height, 0);

   
}