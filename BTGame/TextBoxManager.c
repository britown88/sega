#include "Managers.h"
#include "CoreComponents.h"
#include "segashared/CheckedMemory.h"
#include "Entities/Entities.h"
#include "WorldView.h"
#include "GameClock.h"
#include "segautils/StandardVectors.h"
#include "segautils/BitTwiddling.h"
#include "RichText.h"
#include "Lua.h"

typedef struct {
   Entity *e;
   StringView name;
   int width, height;
   vec(StringPtr) *queue;

   bool done;
   int currentLine, currentChar;
   vec(RichTextLine) *lines;
   String *workingLine;

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
   vecDestroy(RichTextLine)(p->lines);
   vecDestroy(StringPtr)(p->queue);
   stringDestroy(p->workingLine);
}

struct TextBoxManager_t {
   Manager m;
   WorldView *view;

   ht(TextBox) *boxTable;
   RichText *rt;
};

ImplManagerVTable(TextBoxManager)

TextBoxManager *createTextBoxManager(WorldView *view) {
   TextBoxManager *out = checkedCalloc(1, sizeof(TextBoxManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(TextBoxManager);

   out->boxTable = htCreate(TextBox)(_textBoxCompare, _textBoxHash, _textBoxDestroy);
   out->rt = richTextCreateFromRaw("");
   return out;
}

void _destroy(TextBoxManager *self) {
   htDestroy(TextBox)(self->boxTable);
   richTextDestroy(self->rt);
   checkedFree(self);
}
void _onDestroy(TextBoxManager *self, Entity *e) {}
void _onUpdate(TextBoxManager *self, Entity *e) {}

void textBoxManagerCreateTextBox(TextBoxManager *self, StringView name, Recti area) {
   TextBox box = { 0 };
   TextComponent tc = { .lines = vecCreate(TextLine)(&textLineDestroy) };
   int y;

   luaUIAddTextArea(self->view->L, name);

   for (y = area.top; y < area.bottom; ++y) {
      vecPushBack(TextLine)(tc.lines, &(TextLine){
         .x = area.left, .y = y, 
         .line = vecCreate(Span)(&spanDestroy)
      });
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
   box.lines = vecCreate(RichTextLine)(&richTextLineDestroy);
   box.queue = vecCreate(StringPtr)(&stringPtrDestroy);
   box.workingLine = stringCreate("");

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
      vecClear(Span)(line->line);
   });
}

static void _renderToLines(TextBoxManager *self, TextBox *tb) {
   const char *str = c_str(*vecBegin(StringPtr)(tb->queue));
   vecClear(RichTextLine)(tb->lines);

   richTextResetFromRaw(self->rt, str);
   richTextRenderToLines(self->rt, tb->width, tb->lines);

   vecRemoveAt(StringPtr)(tb->queue, 0);
}

static void _updateEntityLines(TextBoxManager *self, TextBox *tb) {
   int line, i;
   TextComponent *tc = entityGet(TextComponent)(tb->e);
   int totalLineCount = vecSize(RichTextLine)(tb->lines);
   int startLine = MAX(0, tb->currentLine - (tb->height - 1));

   for ( i = 0,          line = startLine;
         i < tb->height && line < totalLineCount && line <= tb->currentLine;
         ++i,            ++line) {
      
      TextLine *tline = vecAt(TextLine)(tc->lines, i);
      RichTextLine rtline = *vecAt(RichTextLine)(tb->lines, line);

      vecClear(Span)(tline->line);

      if (line != tb->currentLine) {
         //copy over the full line
         richTextLineCopy(rtline, tline->line);
      }
      else {
         //midline, copy partial
         Span *iter = NULL;
         size_t current = 0;

         for (iter = vecBegin(Span)(rtline); 
            iter != vecEnd(Span)(rtline); ++iter) {

            size_t spanLen = stringLen(iter->string);
            if ((int)current + (int)spanLen < tb->currentChar) {
               //the full span fits so add it
               vecPushBack(Span)(tline->line, &(Span){
                  .style = { iter->style.flags, iter->style.fg, iter->style.bg },
                  .string = stringCopy(iter->string)
               });
               current += spanLen;
            }
            else {
               //partial, build a partial of the string and add a span for it
               size_t partialLen = tb->currentChar - current;
               String *partial = stringCreate("");

               stringConcatEX(partial, c_str(iter->string), partialLen);
               vecPushBack(Span)(tline->line, &(Span){
                  .style = { iter->style.flags, iter->style.fg, iter->style.bg },
                  .string = partial
               });

               //last one, bail out
               break;
            }
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
   else if(!vecIsEmpty(RichTextLine)(tb->lines)){
      //we're drawing so gogo
      Microseconds time = gameClockGetTime(self->view->gameClock);
      if (time >= tb->nextChar) {
         RichTextLine rtline = *vecAt(RichTextLine)(tb->lines, tb->currentLine);
         String *line = tb->workingLine;
         char *c = NULL;
         Milliseconds delay = 0;

         richTextLineGetRaw(rtline, tb->workingLine);
         c = (char*)c_str(tb->workingLine) + tb->currentChar;

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
            if (tb->currentLine + 1 >= (int)vecSize(RichTextLine)(tb->lines)) {
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