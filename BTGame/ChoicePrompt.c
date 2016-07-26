#include "ChoicePrompt.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "RichText.h"
#include "CoreComponents.h"

#include <stdio.h>

struct ChoicePrompt_t {
   Manager m;
   WorldView *view;
   bool enabled;

   Entity *linesEntity;
   RichText *rt;
   vec(RichTextLine) *rtLines;
   vec(StringPtr) *choices;
};

static void _createLinesEntity(ChoicePrompt *self) {
   Entity *e = entityCreate(self->view->entitySystem);
   TextComponent tc = { .lines = vecCreate(TextLine)(&textLineDestroy) };

   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   COMPONENT_ADD(e, VisibilityComponent, .shown = self->enabled);
   entityAdd(TextComponent)(e, &tc);
   entityUpdate(e);

   self->linesEntity = e;
}


ChoicePrompt *createChoicePrompt(WorldView *view) {
   ChoicePrompt *out = checkedCalloc(1, sizeof(ChoicePrompt));
   out->view = view;

   out->rt = richTextCreateFromRaw("");
   out->rtLines = vecCreate(RichTextLine)(&richTextLineDestroy);
   out->enabled = false;
   out->choices = NULL;

   _createLinesEntity(out);

   return out;
}

void choicePromptDestroy(ChoicePrompt *self) {
   richTextDestroy(self->rt);
   vecDestroy(RichTextLine)(self->rtLines);

   if (self->choices) {
      vecDestroy(StringPtr)(self->choices);
   }

   checkedFree(self);
}

static void _renderChoicesToLines(ChoicePrompt *self) {
   VisibilityComponent *vc = entityGet(VisibilityComponent)(self->linesEntity);
   TextComponent *tc = entityGet(TextComponent)(self->linesEntity);
   String *renderedChoices = stringCreate("");
   int i = 0;
   int choiceCount = vecSize(StringPtr)(self->choices);
   int rtLineCount = 0;
   static char buff[128];

   vc->shown = true;   

   //iterate over our choices into a formatted single string which we'll feed to
   //richtext
   for (i = 0; i < choiceCount; ++i) {
      const char *fmt = i ? "\n%i: %s" : "%i: %s";
      sprintf(buff, fmt, i + 1, c_str(*vecAt(StringPtr)(self->choices, i)));
      stringConcat(renderedChoices, buff);
   }

   //render to richtext (parses)
   richTextResetFromRaw(self->rt, c_str(renderedChoices));
   stringDestroy(renderedChoices);

   vecClear(RichTextLine)(self->rtLines);
   richTextRenderToLines(self->rt, 0, self->rtLines);

   rtLineCount = vecSize(RichTextLine)(self->rtLines);

   //clear the old entity list
   vecClear(TextLine)(tc->lines);

   //now we need to push each rtline into the entity
   for (i = 0; i < rtLineCount; ++i) {
      TextLine *line = NULL;
      vecPushBack(TextLine)(tc->lines, &(TextLine){
         .x = 0, .y = i,
         .line = vecCreate(Span)(&spanDestroy)
      });

      //copy from our rt list to the entity
      richTextLineCopy(*vecAt(RichTextLine)(self->rtLines, i), vecAt(TextLine)(tc->lines, i)->line);
   }

}

void choicePromptUpdate(ChoicePrompt *self) {
}

//assumes ownership of the vector
void choicePromptSetChoices(ChoicePrompt *self, vec(StringPtr) *choices) {
   if (self->choices) {
      vecDestroy(StringPtr)(self->choices);
   }

   self->choices = choices;
   self->enabled = true;
   
   _renderChoicesToLines(self);
}

//returns nonzero if event occurs
int choicePromptHandleMouseEvent(ChoicePrompt *self, MouseEvent *e) {
   return 0;
}

//returns nonzero if event occurs
int choicePromptHandleKeyEvent(ChoicePrompt *self, KeyboardEvent *e) {
   return 0;
}

//returns NULL if no selection is yet made
const char *choicePromptGetDecision(ChoicePrompt *self) {
   return NULL;
}

