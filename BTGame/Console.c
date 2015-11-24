#include "Console.h"
#include "segalib/EGA.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "segautils/StandardVectors.h"

#define LINE_COUNT 17
#define BOTTOM 19
#define LEFT 2

struct Console_t {
   WorldView *view;
   Entity *e;
   bool enabled;
   vec(StringPtr) *queue, *inputHistory;
   String *input;
   int historyLocation;
};

Console *consoleCreate(WorldView *view) {
   Console *out = checkedCalloc(1, sizeof(Console));
   out->view = view;
   out->queue = vecCreate(StringPtr)(&stringPtrDestroy);
   out->inputHistory = vecCreate(StringPtr)(&stringPtrDestroy);
   out->input = stringCreate("");
   return out;
}
void consoleDestroy(Console *self) {
   stringDestroy(self->input);
   vecDestroy(StringPtr)(self->queue);
   vecDestroy(StringPtr)(self->inputHistory);
   checkedFree(self);
}

static String *_inputLine(Console *self) {
   return vecAt(TextLine)(entityGet(TextComponent)(self->e)->lines, 0)->text;
}

static void _updateInputLine(Console *self) {
   static const char *cursor = "> ";

   int inlen = stringLen(self->input);
   int len = MIN(34, inlen);//fuck it magic number
   stringSet(_inputLine(self), cursor);

   stringConcat(_inputLine(self), c_str(self->input) + (inlen - len));
}

static void _updateConsoleLines(Console *self) {
   size_t i;
   size_t queuelen = vecSize(StringPtr)(self->queue);
   TextComponent *tc = entityGet(TextComponent)(self->e);
   
   for (i = 0; i < LINE_COUNT; ++i) {
      TextLine *line = vecAt(TextLine)(tc->lines, i + 1);
      if (queuelen > 0 && i < queuelen) {
         String *queueline = *vecAt(StringPtr)(self->queue, i);
         stringSet(line->text, c_str(queueline));
      }
      else {
         stringClear(line->text);
      }
   }
}

static void _processInput(Console *self, String *input) {
   String *str = stringCreate("> ");
   stringConcat(str, c_str(input));   
   consolePushLine(self, c_str(str));
   stringDestroy(str);
}

static void _commitInput(Console *self) {
   String *input;
   
   if (stringLen(self->input) == 0) {
      //input is empty, do nothing
      return;
   }
   //copy out the input
   input = stringCopy(self->input);

   //process the input...
   _processInput(self, input);
   
   //clear the console
   stringClear(self->input);
   _updateInputLine(self);

   //update the history
   vecPushBack(StringPtr)(self->inputHistory, &input);
   self->historyLocation = vecSize(StringPtr)(self->inputHistory);
}

void consoleCreateLines(Console *self) {
   int y;
   Entity *e = entityCreate(self->view->entitySystem);
   TextComponent tc = { .bg = 0, .fg = 15, .lines = vecCreate(TextLine)(&textLineDestroy) };
   int bottomLine = BOTTOM;//last line on screen
   int topLine = bottomLine - (LINE_COUNT + 1);//account for input line

   for (y = bottomLine; y > topLine; --y) {
      String *str = stringCreate("");
      vecPushBack(TextLine)(tc.lines, &(TextLine){LEFT, y, str});
   }

   self->e = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(self->e, LayerComponent, LayerConsole);
   COMPONENT_ADD(self->e, RenderedUIComponent, 0);
   COMPONENT_ADD(self->e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(self->e, &tc);
   entityUpdate(self->e);

   consolePushLine(self, "---------------------------");
   consolePushLine(self, "| Welcome to the console! |");
   consolePushLine(self, "---------------------------");
   consolePushLine(self, "");
   _updateInputLine(self);
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
static void _historyUp(Console *self) {
   size_t historyLen = vecSize(StringPtr)(self->inputHistory);

   if (historyLen > 0 && self->historyLocation > 0) {
      --self->historyLocation;
      stringSet(self->input, c_str(*vecAt(StringPtr)(self->inputHistory, self->historyLocation)));
      _updateInputLine(self);
   }
}
static void _historyDown(Console *self) {
   int historyLen = vecSize(StringPtr)(self->inputHistory);

   if (historyLen > 0){
      if (self->historyLocation < historyLen - 1) {
         ++self->historyLocation;
         stringSet(self->input, c_str(*vecAt(StringPtr)(self->inputHistory, self->historyLocation)));         
      }
      else if (self->historyLocation == historyLen - 1){
         ++self->historyLocation;
         stringClear(self->input);
      }

      _updateInputLine(self);
   }
}

void consoleInputKey(Console *self, SegaKeys key) {
   switch (key) {
   case SegaKey_Enter:
      _commitInput(self);
      break;
   case SegaKey_Up:
      _historyUp(self);
      break;
   case SegaKey_Down:
      _historyDown(self);      
      break;      
   }
}
void consolePushLine(Console *self, const char *line) {
   String *str = stringCreate(line);
   vecInsert(StringPtr)(self->queue, 0, &str);
   _updateConsoleLines(self);
}