#include "TextArea.h"
#include "WorldView.h"
#include "RichText.h"
#include "Managers.h"
#include "Lua.h"
#include "RenderHelpers.h"
#include "segalib/EGA.h"

#include "segautils/StandardVectors.h"
#include "segautils/BitTwiddling.h"

#include "segashared/CheckedMemory.h"

typedef struct {   //Global Text Area
   StringView id;
   TextArea *area;
}GTA;

#define HashTableT GTA
#include "segautils\HashTable_Create.h"

static int _gtaBoxCompare(GTA *e1, GTA *e2) {
   return e1->id == e2->id;
}

static size_t _gtaBoxHash(GTA *p) {
   return hashPtr((void*)p->id);
}

static void _gtaBoxDestroy(GTA *p) {
   textAreaDestroy(p->area);
}

struct TextAreaManager_t {
   WorldView *view;

   ht(GTA) *areaTable;
};

struct TextArea_t {
   short x, y, width, height;

   vec(StringPtr) *queue;
   bool done, shown;
   int currentLine, currentChar;

   RichText *rt;
   vec(RichTextLine) *lines, *shownLines;
   String *workingLine;

   Microseconds nextChar;
   Milliseconds textSpeed;
};


TextAreaManager *textAreaManagerCreate(WorldView *view) {
   TextAreaManager *out = checkedCalloc(1, sizeof(TextAreaManager));
   out->view = view;

   out->areaTable = htCreate(GTA)(_gtaBoxCompare, _gtaBoxHash, _gtaBoxDestroy);

   return out;
}
void textAreaManagerDestroy(TextAreaManager *self) {
   htDestroy(GTA)(self->areaTable);
   checkedFree(self);
}
TextArea *textAreaManagerGet(TextAreaManager *self, StringView id) {
   GTA *found = htFind(GTA)(self->areaTable, &(GTA){.id = id });
   if (found) {
      return found->area;
   }
   return NULL;
}

void textAreaManagerRegister(TextAreaManager *self, StringView id, TextArea *area) {
   GTA *found = htFind(GTA)(self->areaTable, &(GTA){.id = id });

   if (found) {
      found->area = area;
   }
   else {
      htInsert(GTA)(self->areaTable, &(GTA){.id = id, .area = area });
   }

   //now sync it with lua
   luaUIAddTextArea(self->view->L, id, area);
}

TextArea *textAreaCreate(short x, short y, short width, short height) {
   TextArea *out = checkedCalloc(1, sizeof(TextArea));
   int i;

   out->x = x;
   out->y = y;
   out->width = width;
   out->height = height;

   if (out->width == 0) {
      out->width = EGA_TEXT_RES_WIDTH - out->x;
   }

   if (out->height == 0) {
      out->height = EGA_TEXT_RES_HEIGHT - out->y;
   }

   out->width = MIN(EGA_TEXT_RES_WIDTH - out->x, out->width);
   out->height = MIN(EGA_TEXT_RES_HEIGHT - out->y, out->height);

   out->done = out->shown = true;

   out->rt = richTextCreateFromRaw("");
   out->lines = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->shownLines = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->queue = vecCreate(StringPtr)(&stringPtrDestroy);

   //create empty shown lines
   for (i = 0; i < out->height; ++i) {
      RichTextLine l = vecCreate(Span)(&spanDestroy);
      vecPushBack(RichTextLine)(out->shownLines, &l);
   }

   out->workingLine = stringCreate("");

   return out;
}
void textAreaDestroy(TextArea *self) {
   vecDestroy(RichTextLine)(self->lines);
   vecDestroy(RichTextLine)(self->shownLines);
   vecDestroy(StringPtr)(self->queue);
   stringDestroy(self->workingLine);
   richTextDestroy(self->rt);
   checkedFree(self);
}
void textAreaSetSpeed(TextArea *self, Milliseconds timePerCharacter) {
   self->textSpeed = timePerCharacter;
}
void textAreaPushText(TextArea *self, const char *msg) {
   String *msgstr = stringCreate(msg);
   vecPushBack(StringPtr)(self->queue, &msgstr);
}
void textAreaSetText(TextArea *self, const char *msg) {
   String *msgstr = stringCreate(msg);
   vecClear(StringPtr)(self->queue);
   vecPushBack(StringPtr)(self->queue, &msgstr);
   self->done = true;
   textAreaUpdate(self);
}
bool textAreaIsDone(TextArea *self) {
   return self->done;
}
void textAreaSetVisibility(TextArea *self, bool visible) {
   self->shown = visible;
}
void textAreaHide(TextArea *self) {
   self->shown = false;
}
void textAreaShow(TextArea *self) {
   self->shown = true;
}

static void _renderNextMessageToLines(TextArea *self) {
   const char *str = c_str(*vecBegin(StringPtr)(self->queue));
   vecClear(RichTextLine)(self->lines);

   richTextResetFromRaw(self->rt, str);
   richTextRenderToLines(self->rt, self->width, self->lines);

   vecRemoveAt(StringPtr)(self->queue, 0);
}

static void _clearShownLines(TextArea *self) {
   vecForEach(RichTextLine, line, self->shownLines, {
      vecClear(Span)(*line);
   });
}

static void _updateLines(TextArea *self) {
   int line, i;
   int totalLineCount = vecSize(RichTextLine)(self->lines);
   int startLine = MAX(0, self->currentLine - (self->height - 1));

   for (i = 0, line = startLine;
   i < self->height && line < totalLineCount && line <= self->currentLine;
      ++i, ++line) {

      RichTextLine *shownLine = vecAt(RichTextLine)(self->shownLines, i);
      RichTextLine rtline = *vecAt(RichTextLine)(self->lines, line);

      vecClear(Span)(*shownLine);

      if (line != self->currentLine) {
         //copy over the full line
         richTextLineCopy(rtline, *shownLine);
      }
      else {
         //midline, copy partial
         Span *iter = NULL;
         size_t current = 0;

         for (iter = vecBegin(Span)(rtline);
         iter != vecEnd(Span)(rtline); ++iter) {

            size_t spanLen = stringLen(iter->string);
            if ((int)current + (int)spanLen < self->currentChar) {
               //the full span fits so add it
               vecPushBack(Span)(*shownLine, &(Span){
                  .style = { iter->style.flags, iter->style.fg, iter->style.bg },
                     .string = stringCopy(iter->string)
               });
               current += spanLen;
            }
            else {
               //partial, build a partial of the string and add a span for it
               size_t partialLen = self->currentChar - current;
               String *partial = stringCreate("");

               stringConcatEX(partial, c_str(iter->string), partialLen);
               vecPushBack(Span)(*shownLine, &(Span){
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

void textAreaUpdate(TextArea *self) {
   GameClock *gameClock = gameClockGet();
   if (self->done) {//not scrolling
      if (!vecIsEmpty(StringPtr)(self->queue)) {
         //next message!
         _clearShownLines(self);
         self->currentChar = 0;
         self->currentLine = 0;
         self->done = false;
         self->nextChar = gameClockGetTime(gameClock);

         _renderNextMessageToLines(self);
         textAreaUpdate(self);//first update
      }
   }
   else if (!vecIsEmpty(RichTextLine)(self->lines)) {
      //we're drawing so gogo
      Microseconds time = gameClockGetTime(gameClock);
      if (time >= self->nextChar) {
         while ((time >= self->nextChar || self->textSpeed == 0) && !self->done) {

            RichTextLine rtline = *vecAt(RichTextLine)(self->lines, self->currentLine);
            String *line = self->workingLine;
            char *c = NULL;
            Milliseconds delay = 0;

            richTextLineGetRaw(rtline, self->workingLine);
            c = (char*)c_str(self->workingLine) + self->currentChar;

            switch (*c) {
            case ' ': break;
            case '\\':
               if (c[1] == 'c') {
                  self->currentChar += 3;
               }
               break;
            case '.': delay = self->textSpeed * 3; break;
            case ',': delay = self->textSpeed * 2; break;
            case ';': delay = self->textSpeed * 2; break;
            default:  delay = self->textSpeed; break;
            }

            self->nextChar = gameClockGetTime(gameClock) + t_m2u(delay) - (time - self->nextChar);

            ++self->currentChar;
            if (self->currentChar >= (int)stringLen(line)) {
               if (self->currentLine + 1 >= (int)vecSize(RichTextLine)(self->lines)) {
                  self->done = true;
               }
               else {
                  self->currentChar = 0;
                  ++self->currentLine;
               }
            }
         }

         _updateLines(self);
      }
   }
}
void textAreaRender(TextArea *self, WorldView *view, Frame *frame) {
   byte x = self->x;
   byte y = self->y;

   if (!self->shown) {
      return;
   }

   vecForEach(RichTextLine, line, self->shownLines, {      
      vecForEach(Span, span, *line, {
         frameRenderSpan(view, frame, &x, &y, span);
      });
      x = self->x;
      ++y;
   });
}