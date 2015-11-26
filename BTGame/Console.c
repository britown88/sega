#include "Console.h"
#include "segalib/EGA.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "CoreComponents.h"

#include "segautils/StandardVectors.h"
#include "GameClock.h"

#include "liblua/lauxlib.h"
#include "liblua/lualib.h"

#include "RichText.h"

#define LINE_COUNT 17
#define WIDTH 36
#define BOTTOM 19
#define LEFT 2

struct Console_t {
   WorldView *view;
   Entity *e;
   bool enabled;
   vec(StringPtr) *inputHistory;   
   String *input;

   RichText *rt;
   vec(RichTextLine) *queue;

   int historyLocation;
   int cursorPos, selectionWidth;
   Microseconds cursorClock;
   int invertCursor;
   
   lua_State *L;
};

Console *consoleCreate(WorldView *view) {
   Console *out = checkedCalloc(1, sizeof(Console));
   out->view = view;
   out->queue = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->inputHistory = vecCreate(StringPtr)(&stringPtrDestroy);
   out->input = stringCreate("");
   out->rt = richTextCreateFromRaw("");

   out->L = luaL_newstate();
   luaL_openlibs(out->L);

   return out;
}
void consoleDestroy(Console *self) {

   lua_close(self->L);
   richTextDestroy(self->rt);
   stringDestroy(self->input);
   vecDestroy(RichTextLine)(self->queue);
   vecDestroy(StringPtr)(self->inputHistory);
   checkedFree(self);
}

static RichTextLine _inputLine(Console *self) {
   return (vecEnd(TextLine)(entityGet(TextComponent)(self->e)->lines) - 1)->line;
}

static void _updateInputLine(Console *self) {
   static const char *prefix = "> ";
   static const int prefixlen = 2;

   int inlen = stringLen(self->input);
   int len = MIN(WIDTH - prefixlen - 1, inlen);
   RichTextLine input = _inputLine(self);
   String *innerString = stringCreate("");

   stringSet(innerString, prefix);
   stringConcat(innerString, c_str(self->input) + (inlen - len));

   if (self->invertCursor) {
      stringConcat(innerString, "[i] [/i]");
   }

   richTextReset(self->rt, innerString);

   vecClear(Span)(input);
   vecForEach(Span, span, richTextGetSpans(self->rt), {
      Span newSpan = {
         .style = { span->style.flags, span->style.fg, span->style.bg },
         .string = stringCopy(span->string)
      };
      vecPushBack(Span)(input, &newSpan);
   });   
}

static void _updateConsoleLines(Console *self) {
   size_t i;
   size_t queuelen = vecSize(RichTextLine)(self->queue);
   size_t drawnCount = MIN(LINE_COUNT, queuelen);
   size_t skipCount = LINE_COUNT - drawnCount;
   TextComponent *tc = entityGet(TextComponent)(self->e);
   
   for (i = 0; i < LINE_COUNT; ++i) {
      TextLine *line = vecAt(TextLine)(tc->lines, i + 1);
      vecClear(Span)(line->line);
      if (i >= skipCount) {
         //render the spans onto the component
         RichTextLine queueline = *(vecEnd(RichTextLine)(self->queue) - (LINE_COUNT - skipCount) + (i - skipCount));

         //push the queueline spans into the console entity
         vecForEach(Span, span, queueline, {
            Span newSpan = {
               .style = { span->style.flags, span->style.fg, span->style.bg },
               .string = stringCopy(span->string)
            };
            vecPushBack(Span)(line->line, &newSpan);
         });
      }
   }
}

static void _printStackItem(Console *self, int index) {
   int t = lua_type(self->L, index);
   switch (t) {

   case LUA_TSTRING:  /* strings */
      consolePrintLine(self, "Returned: [c=0,5]'%s'[/c]", lua_tostring(self->L, index));
      break;

   case LUA_TBOOLEAN:  /* booleans */
      consolePrintLine(self, "Returned: [c=0,5]%s[/c]", lua_toboolean(self->L, index) ? "true" : "false");
      break;

   case LUA_TNUMBER:  /* numbers */
      consolePrintLine(self, "Returned: [c=0,5]%g[/c]", lua_tonumber(self->L, index));
      break;

   default:  /* other values */
      consolePrintLine(self, "Returned: [c=0,5]%s[/c]", lua_typename(self->L, t));
      break;
   }
}

static void _processInput(Console *self, String *input) {
   int error = luaL_loadbuffer(self->L, c_str(input), stringLen(input), "line") || lua_pcall(self->L, 0, LUA_MULTRET, 0);

   consolePrintLine(self, "> %s", c_str(input));

   if (error) {
      consolePrintLine(self, "[c=0,13]%s[/c]", lua_tostring(self->L, -1));
      lua_pop(self->L, 1);
   }
   else {
      while (lua_gettop(self->L)) {
         _printStackItem(self, 1);
         lua_pop(self->L, 1);
      }
   }
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
   TextComponent tc = { .lines = vecCreate(TextLine)(&textLineDestroy) };
   int bottomLine = BOTTOM;//last line on screen
   int topLine = bottomLine - LINE_COUNT - 1;//account for input line

   for (y = topLine; y <= bottomLine; ++y) {
      vecPushBack(TextLine)(tc.lines, &(TextLine){
         .x = LEFT, .y = y,
         .line = vecCreate(Span)(&spanDestroy)
      });
   }

   self->e = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(self->e, LayerComponent, LayerConsole);
   COMPONENT_ADD(self->e, RenderedUIComponent, 0);
   COMPONENT_ADD(self->e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(self->e, &tc);
   entityUpdate(self->e);

   consolePushLine(self, 
      "---------------------------\n"
      "| [i][c=0,14]Welcome[/i] to the console![/c] |\n"
      "---------------------------");

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
   self->cursorPos += 1;
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

static void _backspace(Console *self) {
   size_t len = stringLen(self->input);
   if (len > 0) {
      stringSubStr(self->input, 0, len - 1);
      _updateInputLine(self);
   }
}

void consoleInputKey(Console *self, SegaKeys key, SegaKeyMods mods) {
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
   case SegaKey_Backspace:
      _backspace(self);
      break;
   }
}

void consolePushLine(Console *self, const char *line) {
   richTextResetFromRaw(self->rt, line);
   richTextRenderToLines(self->rt, WIDTH, self->queue); 
   _updateConsoleLines(self);
}

void consoleUpdate(Console *self) {
   if (!self->cursorClock) {
      self->cursorClock = gameClockGetTime(self->view->gameClock);
   }
   else if (gameClockGetTime(self->view->gameClock) > self->cursorClock) {
      self->cursorClock += t_m2u(500);
      self->invertCursor = !self->invertCursor;
      _updateInputLine(self);
   }
}