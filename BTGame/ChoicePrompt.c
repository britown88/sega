#include "ChoicePrompt.h"
#include "Entities/Entities.h"
#include "segashared/CheckedMemory.h"
#include "RichText.h"
#include "CoreComponents.h"

struct ChoicePrompt_t {
   Manager m;
   WorldView *view;
   bool enabled;

   Entity *linesEntity;
   RichText *rt;
   vec(RichTextLine) *rtLines;
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

   _createLinesEntity(out);

   return out;
}

void choicePromptDestroy(ChoicePrompt *self) {
   richTextDestroy(self->rt);
   vecDestroy(RichTextLine)(self->rtLines);

   checkedFree(self);
}

