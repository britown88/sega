#include "Managers.h"
#include "CoreComponents.h"
#include "segashared/CheckedMemory.h"
#include "Entities/Entities.h"
#include "WorldView.h"
#include "GameClock.h"
#include "segautils/StandardVectors.h"
#include "segautils/BitTwiddling.h"

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
   char *msg = (char*)c_str(*vecBegin(StringPtr)(tb->queue));
   char c = 0;
   static char buff[256] = { 0 };
   int index = 0;
   int lastSpace = 0;

   vecClear(StringPtr)(tb->lines);   

   while (c = *msg++) {
      switch (c) {
      case '\r':
         break;
      case '\\':
         break;
      case '\n':
         lastSpace = index;
         break;
      case '\t':
         lastSpace = index;
         break;      
      case ' ':
         lastSpace = index;
      default:
         buff[index] = c;
         break;
      }
      ++index;
      if (index >= tb->width) {
         String *str;
         buff[index] = 0;
         str = stringCreate(buff);
         vecPushBack(StringPtr)(tb->lines, &str);
         index = lastSpace = 0;
      }
   }

   if (index > 0) {
      String *str;
      buff[index] = 0;
      str = stringCreate(buff);
      vecPushBack(StringPtr)(tb->lines, &str);
   }

   vecRemoveAt(StringPtr)(tb->queue, 0);
}

static void _updateEntityLines(TextBoxManager *self, TextBox *tb) {
   int line, i;
   TextComponent *tc = entityGet(TextComponent)(tb->e);

   for ( i = 0,          line = tb->currentLine; 
         i < tb->height && line < vecSize(StringPtr)(tb->lines); 
         ++i,            ++line) {

      TextLine *tline = vecAt(TextLine)(tc->lines, i);
      String *str = *vecAt(StringPtr)(tb->lines, line);
      const char *lineToDraw = c_str(str);

      if (i < tb->height - 1) {
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
      if (gameClockGetTime(self->view->gameClock) >= tb->nextChar) {
         String *line = *vecAt(StringPtr)(tb->lines, tb->currentLine);
         ++tb->currentChar;
         if (tb->currentChar >= stringLen(line)) {
            tb->currentChar = 0;
            ++tb->currentLine;

            if (tb->currentLine >= vecSize(StringPtr)(tb->lines)) {
               tb->done = true;
            }
         }
         tb->nextChar += t_m2u(100);
         _updateEntityLines(self, tb);
      }      
   }
}

void textBoxManagerUpdate(TextBoxManager *self) {
   htForEach(TextBox, tb, self->boxTable, {
      _updateTextBox(self, tb);
   });
}