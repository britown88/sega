#include "RichText.h"
#include "segalib/EGA.h"

#include "segashared/CheckedMemory.h"
#include "segautils/BitTwiddling.h"

#include <stdio.h>

void spanDestroy(Span *self) {
   stringDestroy(self->string);
}

static void _spanCreate(char *str, size_t len, SpanStyles style, byte bg, byte fg, Span *out) {
   *out = (Span) { .string = stringCreate(""), .style = { .flags = style,.bg = bg,.fg = fg } };
   stringConcatEX(out->string, str, len);
}

void spanRenderToString(Span *self, String *out) {
   static char buff[32] = { 0 };

   if (self->style.flags&Style_Color) {
      sprintf(buff, "[c=%d,%d]", self->style.bg, self->style.fg);
      stringConcat(out, buff);
   }

   if (self->style.flags&Style_Invert) {
      stringConcat(out, "[i]");
   }

   stringConcat(out, c_str(self->string));

   if (self->style.flags&Style_Invert) {
      stringConcat(out, "[\\i]");
   }

   if (self->style.flags&Style_Color) {
      stringConcat(out, "[\\c]");
   }
}

#define VectorTPart Span
#include "segautils/Vector_Impl.h"

#define VectorTPart RichTextLine
#include "segautils/Vector_Impl.h"

#define VectorT SpanStyle
#include "segautils/Vector_Create.h"

void richTextLineDestroy(RichTextLine *self) {
   vecDestroy(Span)(*self);
}

RichTextLine richTextLineCopy(RichTextLine self) {
   RichTextLine out = vecCreate(Span)(&spanDestroy);
   vecForEach(Span, span, self, {
      Span newSpan = {
         .style = {span->style.flags, span->style.fg, span->style.bg},
         .string = stringCopy(span->string)
      };

      vecPushBack(Span)(out, &newSpan);
   });

   return out;
}

void richTextLineGetRaw(RichTextLine self, String *out) {
   stringClear(out);

   vecForEach(Span, span, self, {
      stringConcat(out, c_str(span->string));
   });
}

struct RichText_t {
   String *inner;
   vec(Span) *spanTable;
   vec(SpanStyle) *styleStack;
};

typedef struct {
   char *str;

   char buffer[256];
   size_t bufferLen;
   bool currentInvert;
}RTParse;

static void _pushStyle(RichText *self, SpanStyle style) {
   vecPushBack(SpanStyle)(self->styleStack, &style);
}

//never pops off the last item
static void _popStyle(RichText *self) {
   if(vecSize(SpanStyle)(self->styleStack) > 1){
      vecPopBack(SpanStyle)(self->styleStack);
   }   
}
static SpanStyle *_topStyle(RichText *self) {
   if (!vecIsEmpty(SpanStyle)(self->styleStack)) {
      return (vecEnd(SpanStyle)(self->styleStack) - 1);
   }
   else {
      return NULL;
   }
}

static void _commitSpan(RichText *self, RTParse *p) {
   Span newSpan;
   SpanStyle *style = NULL;
   
   if (p->bufferLen == 0) {
      return;
   }

   style = _topStyle(self);
   _spanCreate(p->buffer, p->bufferLen, style->flags, style->bg, style->fg, &newSpan);
   vecPushBack(Span)(self->spanTable, &newSpan);

   p->bufferLen = 0;
}

static void _skipWhitespace(RTParse *p) {
   char c = 0;
   while (c = *p->str++) {
      if (c == ' ') {
         continue;
      }
      --p->str;
      break;
   }
}

#define BUFF_SIZE 8

static int _readNumber(RTParse *p) {
   char c;
   static char buff[BUFF_SIZE] = { 0 };
   size_t buffLen = 0;
   int out = 0;
   while ((c = *p->str++) && buffLen < BUFF_SIZE) {
      if (c == ' ' || c == ',') {
         break;
      }

      if (c == ']') {
         --p->str;
         break;
      }

      if (c >= '0' && c <= '9') {
         buff[buffLen++] = c;
      }
      else {
         return -1;
      }
   }

   if (buffLen > 0) {
      buff[buffLen] = 0;
      if (sscanf(buff, "%d", &out) > 0) {
         return out;
      }
   }

   return -1;
}

static void _popColor(RichText *self, RTParse *p) {
   SpanStyle *style;
   _popStyle(self);

   style = _topStyle(self);
   if (!p->currentInvert) {
      style->flags &= ~Style_Invert;
   }
   else {
      style->flags |= Style_Invert;
   }
}

static void _pushColor(RichText *self, RTParse *p, byte bg, byte fg) {
   SpanStyle newStyle = { 0 };

   newStyle.flags |= Style_Color;
   newStyle.bg = bg;
   newStyle.fg = fg;

   if (p->currentInvert) {
      newStyle.flags |= Style_Invert;
   }

   _pushStyle(self, newStyle);
}

//parse a single tag
static int _parseTag(RichText *self, RTParse *p) {
   char c = 0, cmd = 0;
   bool end = false;
   _skipWhitespace(p);
   if (cmd = *p->str++) {
      if (cmd == '/') {
         end = true;
         cmd = *p->str++;
      }

      if (cmd == 'i' || cmd == 'I') {
         //invert command
         _skipWhitespace(p);
         c = *p->str++;
         if (c != ']') { 
            return 1;
         }
         else {
            //commit the invert
            SpanStyle *style = _topStyle(self);
            p->currentInvert = !end;
            if (end) {
               style->flags &= ~Style_Invert;
            }
            else {
               style->flags |= Style_Invert;
            }
            return 0;
         }
      }
      else if (cmd == 'c' || cmd == 'C') {
         int bg = 0, fg = 0;

         //color command
         _skipWhitespace(p);
         c = *p->str++;
         if (end && c == ']') {            
            //end color
            _popColor(self, p);
            return 0;
         }
         else if (c != '=') {
            return 1;
         }

         //get bg
         _skipWhitespace(p);
         bg = _readNumber(p);
         if (bg < 0) {
            return 1;
         }

         //get fg
         _skipWhitespace(p);
         fg = _readNumber(p);
         if (fg < 0) {
            return 1;
         }
         //get closing bracket
         _skipWhitespace(p);
         c = *p->str++;
         if (c != ']') {
            return 1;
         }
         else {
            //commit the color
            _pushColor(self, p, bg, fg);
            return 0;
         }
      }
   }

   return 1;//bad char
}

//rebuild span table from scratch based on current inner
static void _rebuildSpans(RichText *self) {
   RTParse p = { (char*)c_str(self->inner) , 0 };
   char c = 0;
   char *startStr = NULL;
   
   vecClear(Span)(self->spanTable);
   vecClear(SpanStyle)(self->styleStack);

   _pushStyle(self, (SpanStyle) { 0 });

   while (c = *p.str++) {
      switch (c) {
      case '[':
         _commitSpan(self, &p);
         startStr = p.str;
         if (_parseTag(self, &p)) {
            p.str = startStr;
            p.buffer[p.bufferLen++] = '[';
         }
         break;
      case '\\':
         if(c = *p.str++) {
            if (c == 'n' || c == 'N') {
               p.buffer[p.bufferLen++] = '\n';
            }
            else {
               p.buffer[p.bufferLen++] = c;
            }
         }
         break;
      default:
         p.buffer[p.bufferLen++] = c;
         break;
      }

      if (p.bufferLen >= 256) {
         _commitSpan(self, &p);
      }
   }

   _commitSpan(self, &p);
}

// This takes ownership of the string!
RichText *richTextCreate(String *string) {
   RichText *out = checkedCalloc(1, sizeof(RichText));
   out->inner = string;
   out->spanTable = vecCreate(Span)(&spanDestroy);
   out->styleStack = vecCreate(SpanStyle)(NULL);
   _rebuildSpans(out);
   return out;
}
RichText *richTextCreateFromRaw(const char *string) {
   return richTextCreate(stringCreate(string));
}

void richTextDestroy(RichText *self) {
   stringDestroy(self->inner);
   vecDestroy(Span)(self->spanTable);
   vecDestroy(SpanStyle)(self->styleStack);
   checkedFree(self);
}

// This takes ownership of the string!
void richTextReset(RichText *self, String *string) {
   stringDestroy(self->inner);
   self->inner = string;
   _rebuildSpans(self);
}
void richTextResetFromRaw(RichText *self, const char *string) {
   richTextReset(self, stringCreate(string));
}

void richTextGet(RichText *self, String *out) {
   stringClear(out);
   stringConcat(out, c_str(self->inner));
}
void richTextGetRaw(RichText *self, String *out) {
   stringClear(out);
   vecForEach(Span, span, self->spanTable, {
      stringConcat(out, c_str(span->string));
   });
}

//pushes lines in order to a string vector with the specified width restriction
//linewidth of 0 will function the same as INF
void richTextRenderToLines(RichText *self, size_t lineWidth, vec(RichTextLine) *outList) {
   Span *iter = NULL;
   Span newSpan = { 0 };

   size_t currentWidth = 0;

   //store the last widthpos of a space
   int lastSpace = 0;

   RichTextLine workingLine = vecCreate(Span)(&spanDestroy);

   //loop over every span
   for ( iter = vecBegin(Span)(self->spanTable); 
         iter < vecEnd(Span)(self->spanTable); ++iter) {
      int spanWidth = stringLen(iter->string);
      char *str = (char*)c_str(iter->string);
      int i = 0;
      int splitPoint = 0;

      //skip empty spans
      if (spanWidth == 0) {
         continue;
      }

      //loop over every character
      for (i = 0; i < spanWidth; ++i, ++currentWidth) {

         if (str[i] == '\n') {
            _spanCreate(str + splitPoint, i - splitPoint, iter->style.flags, iter->style.bg, iter->style.fg, &newSpan);
            vecPushBack(Span)(workingLine, &newSpan);
            vecPushBack(RichTextLine)(outList, &workingLine);
            workingLine = vecCreate(Span)(&spanDestroy);
            
            if (++i == spanWidth) {
               break;
            }

            currentWidth = 0;
            splitPoint = i;            
         }

         //we're over our line length, need to split
         if (currentWidth >= lineWidth) {

            int subStringWidth = i - splitPoint; // number of characters in the current working span segment
            int subStringStartPos = currentWidth - subStringWidth; //line index of the first character in our current substring

            int segmentLength = 0;// number of characters we are going to copy out into a span

            if (lastSpace >= subStringStartPos) {
               segmentLength = lastSpace - subStringStartPos + 1;//add one to include the space
            } 
            else {
               segmentLength = subStringWidth;
            }

            //create that span, push it to the workingline, and push the workingline
            _spanCreate(str + splitPoint, segmentLength, iter->style.flags, iter->style.bg, iter->style.fg, &newSpan);
            vecPushBack(Span)(workingLine, &newSpan);
            vecPushBack(RichTextLine)(outList, &workingLine);
            workingLine = vecCreate(Span)(&spanDestroy);

            //and then move width forward the number of characters that got carried over onto the next line
            currentWidth = subStringWidth - segmentLength;

            //our alst space on a new line is always 0
            lastSpace = -1;

            //and lastly move our split index forward by the number of character we carved out
            splitPoint += segmentLength;

         }

         if (str[i] == ' ') {
            lastSpace = currentWidth;
         }
         
      }

      //push the remaining portions of this span onto the current working line
      if (splitPoint < spanWidth) {
         _spanCreate(str + splitPoint, spanWidth - splitPoint, iter->style.flags, iter->style.bg, iter->style.fg, &newSpan);
         vecPushBack(Span)(workingLine, &newSpan);
      }
   }

   if (!vecIsEmpty(Span)(workingLine)) {
      vecPushBack(RichTextLine)(outList, &workingLine);
      workingLine = vecCreate(Span)(&spanDestroy);
   }

   vecDestroy(Span)(workingLine);
}

//// >=D

//
//static char _colorCodeFromText(byte bg, byte fg) {
//   byte abg = 0, afg = 0;
//   if (bg >= '0' && bg <= '9') { abg = bg - '0'; }
//   else if (bg >= 'A' && bg <= 'F') {  abg = bg - 'A' + 10; }
//   else if (bg >= 'a' && bg <= 'f') { abg = bg - 'a' + 10; }
//
//   if (fg >= '0' && fg <= '9') { afg = fg - '0'; }
//   else if (fg >= 'A' && fg <= 'F') { afg = fg - 'A' + 10; }
//   else if (fg >= 'a' && fg <= 'f') { afg = fg - 'a' + 10; }
//
//   return textGenerateColorCode(abg, afg);
//}
//
//void stringRenderToArea(const char *str, size_t lineWidth, vec(StringPtr) *outList) {
//   char *msg = (char*)str;
//   char c = 0;
//   static char buff[256] = { 0 };
//   size_t index = 0;
//   size_t lastSpace = 0;
//   size_t skippedChars = 0;
//   size_t skippedSinceLastSpace = 0;
//   size_t spaceOnLastSkip = 0;
//   byte lastColor = 0;
//   bool colorChanged = false;
//
//   while (c = *msg++) {
//      switch (c) {
//      case '\r':
//         break;
//      case '\\':
//         c = *msg++;
//         if (c && c == 'c') {
//            byte bg = *msg++;
//            byte fg = *msg++;
//            buff[index++] = '\\';
//            buff[index++] = 'c';
//            buff[index] = _colorCodeFromText(bg, fg);
//            lastColor = buff[index];
//            colorChanged = true;
//            skippedChars += 3;
//
//            if (spaceOnLastSkip != lastSpace) {
//               spaceOnLastSkip = lastSpace;
//               skippedSinceLastSpace = 0;
//            }
//            skippedSinceLastSpace += 3;
//         }
//         break;
//      case '\n':
//         lastSpace = index;
//         break;
//      case '\t':
//         lastSpace = index;
//         break;
//      case ' ':
//         lastSpace = index;
//      default:
//         buff[index] = c;
//         break;
//      }
//      ++index;
//      if (index - skippedChars >= lineWidth) {
//         String *str;
//         buff[lastSpace] = 0;
//         str = stringCreate(buff);
//         vecPushBack(StringPtr)(outList, &str);
//         index -= lastSpace + 1;
//         memcpy(buff, buff + lastSpace + 1, index);
//
//         if (lastSpace == spaceOnLastSkip) {
//            skippedChars = skippedSinceLastSpace;
//         }
//         else {
//            if (colorChanged) {               
//               skippedChars = 3;
//               memmove(buff + 3, buff, index);
//               buff[0] = '\\';
//               buff[1] = 'c';
//               buff[2] = lastColor;
//               index += 3;
//            }
//            else {
//               skippedChars = 0;
//            }
//         }
//
//         //reset crap
//         lastSpace = 0;         
//         skippedSinceLastSpace = 0;
//         spaceOnLastSkip = 0;
//      }
//   }
//
//   if (index - skippedChars > 0) {
//      String *str;
//      buff[index] = 0;
//      str = stringCreate(buff);
//      vecPushBack(StringPtr)(outList, &str);
//   }
//}