#include "EGATexture.h"
#include "defined_gl.h"
#include "segashared\CheckedMemory.h"
#include "segautils\BitTwiddling.h"

#define HANDLE_COUNT 8

typedef struct EGATexture_t {
   GLuint handle[HANDLE_COUNT];
   int currentHandle;
	byte *pixels;
};

EGATexture *egaTextureCreate(){
   int i;
   EGATexture *r = checkedCalloc(1, sizeof(EGATexture));
	r->pixels = checkedCalloc(1, EGA_RES_WIDTH*EGA_RES_HEIGHT * 4);

   glGenTextures(HANDLE_COUNT, r->handle);
   for (i = 0; i < HANDLE_COUNT; ++i){
      glBindTexture(GL_TEXTURE_2D, r->handle[i]);
      glTexImage2D(
         GL_TEXTURE_2D, 0, GL_RGBA8,
         EGA_RES_WIDTH, EGA_RES_HEIGHT,
         0, GL_RGBA, GL_UNSIGNED_BYTE, r->pixels);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   }

	glBindTexture(GL_TEXTURE_2D, 0);

	return r;
}
void egaTextureDestroy(EGATexture *self) {

	glDeleteTextures(HANDLE_COUNT, self->handle);
	checkedFree(self->pixels);
	checkedFree(self);
}

static void _updatePixels(EGATexture *self, Frame *frame, byte *palette){
	int x, y, i;
	byte *cData = self->pixels;

	for (y = 0; y < EGA_RES_HEIGHT; ++y){
		for (x = 0; x < EGA_RES_WIDTH; ++x) {

			byte color = 0;
			byte cbytes[4] = { 0 };
			int colori = 0;

			for (i = 0; i < EGA_PLANES; ++i) {
				setBit(&color, i, getBitFromArray(frame->planes[i].lines[y].pixels, x));
			}

			colori = getEGAColor(palette[color]);
			memcpy(cbytes, &colori, 4);

			for (i = 0; i < 4; ++i){
				*cData++ = cbytes[i];
			}
		}
	}
}

void egaTextureRenderFrame(EGATexture *self, Frame *frame, byte *palette){
	_updatePixels(self, frame, palette);

	//glFinish();
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, self->handle[self->currentHandle++]);
   if (self->currentHandle == HANDLE_COUNT){
      self->currentHandle = 0;
   }

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, EGA_RES_WIDTH, EGA_RES_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, self->pixels);

	glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(EGA_RES_WIDTH, EGA_RES_HEIGHT, 0.0f);  // Bottom Left Of The Texture and Quad
		glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, EGA_RES_HEIGHT, 0.0f);  // Bottom Right Of The Texture and Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f);  // Top Right Of The Texture and Quad
		glTexCoord2f(1.0f, 0.0f); glVertex3f(EGA_RES_WIDTH, 0.0f, 0.0f); // Top Left Of The Texture and Quad
	glEnd();

}
