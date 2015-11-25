#include "RichText.h"
#include "segalib/EGA.h"

#include "segashared/CheckedMemory.h"
#include "segautils/BitTwiddling.h"
#include <stdio.h>

typedef enum {
   Style_Color = (1 << 0),
   Style_Invert = (1 << 1)
}SpanStyles;

typedef struct {
   SpanStyles flags;
   byte fg, bg;
}SpanStyle;

typedef struct {
   SpanStyle style;
   String *string;
} Span;


static void _spanDestroy(Span *self) {
   stringDestroy(self->string);
}

static void _spanCreate(char *str, size_t len, SpanStyles style, byte bg, byte fg, Span *out) {
   *out = (Span) { .string = stringCreate(""), .style = { .flags = style,.bg = bg,.fg = fg } };
   stringConcatEX(out->string, str, len);
}

static void _spanRenderToString(Span *self, String *out) {
   static char buff[32] = { 0 };

   if (self->style.flags&Style_Color) {
      sprintf(buff, "[c%c%c]", asciiFrom4BitHex(self->style.bg), asciiFrom4BitHex(self->style.fg));
      stringConcat(out, buff);
   }

   if (self->style.flags&Style_Invert) {
      stringConcat(out, "[i]");
   }

   stringConcat(out, c_str(self->string));

   if (self->style.flags&Style_Color) {
      stringConcat(out, "[\\c]");
   }

   if (self->style.flags&Style_Invert) {
      stringConcat(out, "[\\i]");
   }
}


#define VectorT Span
#include "segautils/Vector_Create.h"

#define VectorT SpanStyle
#include "segautils/Vector_Create.h"

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
      switch (c) {
      case ' ':
      case '\n':
      case '\t':
         continue;
      default:
         break;
      }
   }
}

#define BUFF_SIZE 8

static int _readNumber(RTParse *p) {
   char c = 0;
   static char buff[BUFF_SIZE] = { 0 };
   size_t buffLen = 0;
   int out = 0;
   while (c = *p->str++ && buffLen < BUFF_SIZE) {
      if (c == ' ' || c == ',') {
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
static void _parseTag(RichText *self, RTParse *p) {
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
            return;
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
         }
         else if (c != '=') {
            return;
         }

         //get bg
         _skipWhitespace(p);
         bg = _readNumber(p);
         if (bg < 0) {
            return;
         }

         //get fg
         _skipWhitespace(p);
         fg = _readNumber(p);
         if (fg < 0) {
            return;
         }
         //get closing bracket
         _skipWhitespace(p);
         c = *p->str++;
         if (c != ']') {
            return;
         }
         else {
            //commit the color
            _pushColor(self, p, bg, fg);
         }
      }
      else {
         return;
      }      
   }
}

//rebuild span table from scratch based on current inner
static void _rebuildSpans(RichText *self) {
   RTParse p = { (char*)c_str(self->inner) , 0 };
   char c = 0;
   
   vecClear(Span)(self->spanTable);
   vecClear(SpanStyle)(self->styleStack);

   _pushStyle(self, (SpanStyle) { 0 });

   while (c = *p.str++) {
      switch (c) {
      case '[':
         _commitSpan(self, &p);
         _parseTag(self, &p);
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
   }

   _commitSpan(self, &p);
}

// This takes ownership of the string!
RichText *richTextCreate(String *string) {
   RichText *out = checkedCalloc(1, sizeof(RichText));
   out->inner = string;
   out->spanTable = vecCreate(Span)(&_spanDestroy);
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
void richTextRenderToLines(RichText *self, size_t lineWidth, vec(StringPtr) *outList) {

}

//same as renderToLines but leaves out any markdown tagging... useful for manual manipulation
void richTextRenderRawToLines(RichText *self, size_t lineWidth, vec(StringPtr) *outList) {

}

void richTextRenderToFrame(Frame *frame, FontFactory *fontFactory, TextComponent *tc) {

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