#include "Managers.h"
#include "CoreComponents.h"
#include "segashared/CheckedMemory.h"
#include "Entities/Entities.h"
#include "WorldView.h"
#include "GameClock.h"
#include "segautils/StandardVectors.h"
#include "segautils/BitTwiddling.h"
#include "TextHelpers.h"

typedef struct {
   Entity *e;
   StringView name;
   int width, height;
   vec(StringPtr) *queue;

   bool done;
   int currentLine, currentChar;
   vec(StringPtr) *lines;

   Microseconds nextChar;
}TextBox;

#define HashTableT TextBox
#include "segautils\HashTable_Create.h"

static int _textBoxCompare(TextBox *e1, TextBox *e2) {
   return e1->name == e2->name;
}

static size_t _textBoxHash(TextBox *p) {
   return hashPtr((void*)p->name);
}

static void _textBoxDestroy(TextBox *p) {
   vecDestroy(StringPtr)(p->lines);
   vecDestroy(StringPtr)(p->queue);
}

struct TextBoxManager_t {
   Manager m;
   WorldView *view;

   ht(TextBox) *boxTable;
};

ImplManagerVTable(TextBoxManager)

TextBoxManager *createTextBoxManager(WorldView *view) {
   TextBoxManager *out = checkedCalloc(1, sizeof(TextBoxManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(TextBoxManager);

   out->boxTable = htCreate(TextBox)(_textBoxCompare, _textBoxHash, _textBoxDestroy);
   return out;
}

void _destroy(TextBoxManager *self) {
   htDestroy(TextBox)(self->boxTable);
   checkedFree(self);
}
void _onDestroy(TextBoxManager *self, Entity *e) {}
void _onUpdate(TextBoxManager *self, Entity *e) {}

void textBoxManagerCreateTextBox(TextBoxManager *self, StringView name, Recti area) {
   TextBox box = { 0 };
   TextComponent tc = { .bg = 0,.fg = 15,.lines = vecCreate(TextLine)(&textLineDestroy) };
   int y;

   for (y = area.top; y < area.bottom; ++y) {
      String *str = stringCreate("");
      vecPushBack(TextLine)(tc.lines, &(TextLine){area.left, y, str});
   }

   box.e = entityCreate(self->view->entitySystem);
   COMPONENT_ADD(box.e, LayerComponent, LayerUI);
   COMPONENT_ADD(box.e, RenderedUIComponent, 0);
   entityAdd(TextComponent)(box.e, &tc);
   entityUpdate(box.e);

   box.done = true;
   box.width = rectiWidth(&area);
   box.height = rectiHeight(&area);
   box.name = name;
   box.lines = vecCreate(StringPtr)(&stringPtrDestroy);
   box.queue = vecCreate(StringPtr)(&stringPtrDestroy);

   htInsert(TextBox)(self->boxTable, &box);
}
void textBoxManagerPushText(TextBoxManager *self, StringView name, const char *msg) {
   TextBox *found = htFind(TextBox)(self->boxTable, &(TextBox){.name = name});
   if (found) {
      String *msgstr = stringCreate(msg);
      vecPushBack(StringPtr)(found->queue, &msgstr);
   }
}

static void _clearLineEntity(Entity *e) {
   TextComponent *tc = entityGet(TextComponent)(e);
   vecForEach(TextLine, line, tc->lines, {
      stringClear(line->text);
   });
}

static void _renderToLines(TextBoxManager *self, TextBox *tb) {
   vecClear(StringPtr)(tb->lines);
   stringRenderToArea(c_str(*vecBegin(StringPtr)(tb->queue)), tb->width, tb->lines);
   vecRemoveAt(StringPtr)(tb->queue, 0);
}

static void _updateEntityLines(TextBoxManager *self, TextBox *tb) {
   int line, i;
   TextComponent *tc = entityGet(TextComponent)(tb->e);
   int totalLineCount = vecSize(StringPtr)(tb->lines);
   int startLine = MAX(0, tb->currentLine - (tb->height - 1));

   for ( i = 0,          line = startLine;
         i < tb->height && line < totalLineCount && line <= tb->currentLine;
         ++i,            ++line) {

      TextLine *tline = vecAt(TextLine)(tc->lines, i);
      String *str = *vecAt(StringPtr)(tb->lines, line);
      const char *lineToDraw = c_str(str);

      if (line != tb->currentLine) {
         //already done
         stringSet(tline->text, lineToDraw);
      }
      else {
         //midline
         if (tline->text) {
            stringClear(tline->text);
            stringConcatEX(tline->text, lineToDraw, tb->currentChar);
         }
         
      }
   }
}

static void _updateTextBox(TextBoxManager *self, TextBox *tb) {
   
   if (tb->done){//not scrolling
      if (!vecIsEmpty(StringPtr)(tb->queue)) {
         //next message!
         _clearLineEntity(tb->e);
         tb->currentChar = 0;
         tb->currentLine = 0;
         tb->done = false;
         tb->nextChar = gameClockGetTime(self->view->gameClock);

         _renderToLines(self, tb);
      }      
   }
   else {
      //we're drawing so gogo
      Microseconds time = gameClockGetTime(self->view->gameClock);
      if (time >= tb->nextChar) {
         String *line = *vecAt(StringPtr)(tb->lines, tb->currentLine);
         char *c = (char*)c_str(*vecAt(StringPtr)(tb->lines, tb->currentLine)) + tb->currentChar;
         Milliseconds delay = 0;

         
         switch (*c) {
         case ' ': break;
         case '\\':
            if (c[1] == 'c') {
               tb->currentChar += 3;
            }
            break;
         case '.': delay = 500; break;
         case ',': delay = 250; break;
         case ';': delay = 250; break;
         default:  delay = 50; break;
         }

         tb->nextChar = gameClockGetTime(self->view->gameClock) + t_m2u(delay) - (time - tb->nextChar);

         ++tb->currentChar;
         if (tb->currentChar >= (int)stringLen(line)) {  
            if (tb->currentLine + 1 >= (int)vecSize(StringPtr)(tb->lines)) {
               tb->done = true;
            }
            else {
               tb->currentChar = 0;
               ++tb->currentLine;
            }
         }

         _updateEntityLines(self, tb);
      }      
   }
}

void textBoxManagerUpdate(TextBoxManager *self) {
   htForEach(TextBox, tb, self->boxTable, {
      _updateTextBox(self, tb);
   });
}