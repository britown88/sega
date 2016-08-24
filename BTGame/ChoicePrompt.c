#include "ChoicePrompt.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "RichText.h"
#include "RenderHelpers.h"

#include <stdio.h>

struct ChoicePrompt_t {
   Manager m;
   WorldView *view;
   bool enabled;

   RichText *rt;
   vec(RichTextLine) *rtLines;
   vec(StringPtr) *choices;
   vec(TextLine) *lines;

   String *selection;
   int selectedIndex;


};

ChoicePrompt *createChoicePrompt(WorldView *view) {
   ChoicePrompt *out = checkedCalloc(1, sizeof(ChoicePrompt));
   out->view = view;

   out->rt = richTextCreateFromRaw("");
   out->rtLines = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->enabled = false;
   out->choices = NULL;
   out->selection = NULL;
   out->selectedIndex = -1;

   out->lines = vecCreate(TextLine)(&textLineDestroy);

   return out;
}

void choicePromptDestroy(ChoicePrompt *self) {
   richTextDestroy(self->rt);
   vecDestroy(RichTextLine)(self->rtLines);
   vecDestroy(TextLine)(self->lines);

   if (self->choices) {
      vecDestroy(StringPtr)(self->choices);
   }

   checkedFree(self);
}

static void _renderChoicesToLines(ChoicePrompt *self) {
   String *renderedChoices = stringCreate("");
   int i = 0;
   int choiceCount = vecSize(StringPtr)(self->choices);
   int rtLineCount = 0;
   static char buff[128];

   //iterate over our choices into a formatted single string which we'll feed to
   //richtext
   for (i = 0; i < choiceCount; ++i) {
      const char *fmt = i ? "\n%i: %s" : "%i: %s";
      const char *hfmt = i ? "[i]\n%i: %s[/i]" : "[i]%i: %s[/i]";
      sprintf(buff, i == self->selectedIndex ? hfmt : fmt, i + 1, c_str(*vecAt(StringPtr)(self->choices, i)));
      stringConcat(renderedChoices, buff);
   }

   //render to richtext (parses)
   richTextResetFromRaw(self->rt, c_str(renderedChoices));
   stringDestroy(renderedChoices);

   vecClear(RichTextLine)(self->rtLines);
   richTextRenderToLines(self->rt, 0, self->rtLines);

   rtLineCount = vecSize(RichTextLine)(self->rtLines);

   //clear the old line list
   vecClear(TextLine)(self->lines);

   //now we need to push each rtline into the linelist
   for (i = 0; i < rtLineCount; ++i) {
      TextLine *line = NULL;
      vecPushBack(TextLine)(self->lines, &(TextLine){
         .x = 0, .y = i,
         .line = vecCreate(Span)(&spanDestroy)
      });

      //copy from our rt list to the line list
      richTextLineCopy(*vecAt(RichTextLine)(self->rtLines, i), vecAt(TextLine)(self->lines, i)->line);
   }

}

static void _setSelected(ChoicePrompt *self) {
   self->selection = *vecAt(StringPtr)(self->choices, self->selectedIndex);
   self->enabled = false;
}

void choicePromptUpdate(ChoicePrompt *self) {
}

void choicePromptRender(ChoicePrompt *self, Frame *frame) {
   if (self->enabled) {
      Font *defaultFont = fontFactoryGetFont(self->view->fontFactory, 0, 15);

      vecForEach(TextLine, line, self->lines, {
         byte x = line->x;
         byte y = line->y;
         vecForEach(Span, span, line->line,{
            frameRenderSpan(self->view, frame, &x, &y, span);
         });
      });
   }
}

//assumes ownership of the vector
void choicePromptSetChoices(ChoicePrompt *self, vec(StringPtr) *choices) {
   if (self->choices) {
      vecDestroy(StringPtr)(self->choices);
   }

   self->choices = choices;
   self->enabled = true;
   self->selection = NULL;
   self->selectedIndex = -1;
   
   _renderChoicesToLines(self);
}

//returns nonzero if event occurs
int choicePromptHandleMouseEvent(ChoicePrompt *self, MouseEvent *e) {
   if (!self->enabled) {
      return 0;
   }

   return 1;
}

//returns nonzero if event occurs
int choicePromptHandleKeyEvent(ChoicePrompt *self, KeyboardEvent *e) {
   if (!self->enabled) {
      return 0;
   }

   int key = e->key - SegaKey_0;

   if (!self->choices || key < 0 || key > vecSize(StringPtr)(self->choices)) {
      return 0;
   }


   if (e->action == SegaKey_Pressed) {
      self->selectedIndex = key - 1;
      _renderChoicesToLines(self);

   }
   else if (e->action == SegaKey_Released) {
      if (key != self->selectedIndex + 1) {
         return 1;
      }
      _setSelected(self);      
   }

   return 1;
}

int choicePromptGetDecisionIndex(ChoicePrompt *self) {
   return self->selectedIndex;
}

//returns NULL if no selection is yet made
const char *choicePromptGetDecision(ChoicePrompt *self) {
   if (self->selection) {
      return c_str(self->selection);
   }

   return NULL;
}

