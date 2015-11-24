#include "Console.h"
#include "segalib/EGA.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "segautils/StandardVectors.h"

#define LINE_COUNT (EGA_TEXT_RES_HEIGHT)

struct Console_t {
   WorldView *view;
   Entity *e;
   bool enabled;
   vec(StringPtr) *queue;
   String *input;
};

Console *consoleCreate(WorldView *view) {
   Console *out = checkedCalloc(1, sizeof(Console));
   out->view = view;
   out->queue = vecCreate(StringPtr)(&stringPtrDestroy);
   out->input = stringCreate("");
   return out;
}
void consoleDestroy(Console *self) {
   stringDestroy(self->input);
   vecDestroy(StringPtr)(self->queue);
   checkedFree(self);
}

static String *_inputLine(Console *self) {
   return vecAt(TextLine)(entityGet(TextComponent)(self->e)->lines, 0)->text;
}

static void _updateInputLine(Console *self) {
   int inlen = stringLen(self->input);
   int len = MIN(EGA_TEXT_RES_WIDTH - 1, inlen);

   stringSet(_inputLine(self), c_str(self->input) + (inlen - len));
}

void consoleCreateLines(Console *self) {
   int y;
   Entity *e = entityCreate(self->view->entitySystem);
   TextComponent tc = { .bg = 0, .fg = 15, .lines = vecCreate(TextLine)(&textLineDestroy) };
   int bottomLine = EGA_TEXT_RES_HEIGHT - 1;//last line on screen
   int topLine = bottomLine - LINE_COUNT;

   for (y = bottomLine; y > topLine; --y) {
      String *str = stringCreate("");
      vecPushBack(TextLine)(tc.lines, &(TextLine){0, y, str});
   }

   self->e = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(self->e, LayerComponent, LayerUI);
   COMPONENT_ADD(self->e, RenderedUIComponent, 0);
   COMPONENT_ADD(self->e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(self->e, &tc);
   entityUpdate(self->e);
}

void consoleSetEnabled(Console *self, bool enabled) {
   self->enabled = enabled;
   entityGet(VisibilityComponent)(self->e)->shown = enabled;
}

bool consoleGetEnabled(Console *self) {
   return self->enabled;
}

void consoleInputChar(Console *self, char c) {
   stringConcatEX(self->input, &c, 1);
   _updateInputLine(self);
}
void consoleInputKey(Console *self, SegaKeys key) {

}
void consolePushLine(Console *self, const char *line) {

}