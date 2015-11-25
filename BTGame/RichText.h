#pragma once

#include "segautils/StandardVectors.h"
#include "segautils/Defs.h"
#include "CoreComponents.h"

typedef struct FontFactory_t FontFactory;

typedef struct RichText_t RichText;

// This takes ownership of the string!
RichText *richTextCreate(String *string);

RichText *richTextCreateFromRaw(const char *string);

void richTextReset(RichText *self, String *string);
void richTextGet(RichText *self, String *out);
void richTextGetRaw(RichText *self, String *out);
void richTextResetFromRaw(RichText *self, const char *string);
void richTextDestroy(RichText *self);

//pushes lines in order to a string vector with the specified width restriction
//linewidth of 0 will function the same as INF
void richTextRenderToLines(RichText *self, size_t lineWidth, vec(StringPtr) *outList);

//same as renderToLines but leaves out any markdown tagging... useful for manual manipulation
void richTextRenderRawToLines(RichText *self, size_t lineWidth, vec(StringPtr) *outList);

void richTextRenderToFrame(Frame *frame, FontFactory *fontFactory, TextComponent *tc);




