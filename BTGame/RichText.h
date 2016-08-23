#pragma once

#include "segautils/StandardVectors.h"
#include "segautils/Defs.h"

typedef struct FontFactory_t FontFactory;

typedef enum {
   Style_Color = (1 << 0),
   Style_Invert = (1 << 1)
}SpanStyles;

typedef struct {
   SpanStyles flags;
   byte fg, bg;
}SpanStyle;

typedef struct Span_t{
   SpanStyle style;
   String *string;
} Span;

void spanRenderToString(Span *self, String *out);
void spanDestroy(Span *self);

#define VectorTPart Span
#include "segautils/Vector_Decl.h"

typedef vec(Span) *RichTextLine;

#define VectorTPart RichTextLine
#include "segautils/Vector_Decl.h"

void richTextLineDestroy(RichTextLine *self);
void richTextLineCopy(RichTextLine self, RichTextLine other);
void richTextLineGetRaw(RichTextLine self, String *out);

typedef struct RichText_t RichText;

// This takes ownership of the string!
RichText *richTextCreate(String *string);
RichText *richTextCreateFromRaw(const char *string);
void richTextDestroy(RichText *self);

void richTextGet(RichText *self, String *out);
void richTextGetRaw(RichText *self, String *out);
void richTextReset(RichText *self, String *string);
void richTextResetFromRaw(RichText *self, const char *string);

RichTextLine richTextGetSpans(RichText *self);

//pushes lines in order to a string vector with the specified width restriction
//linewidth of 0 will function the same as INF
void richTextRenderToLines(RichText *self, size_t lineWidth, vec(RichTextLine) *outList);





