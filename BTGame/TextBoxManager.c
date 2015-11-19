#include "Managers.h"
#include "CoreComponents.h"
#include "segashared/CheckedMemory.h"
#include "Entities/Entities.h"
#include "WorldView.h"
#include "segautils/StandardVectors.h"
#include "segautils/BitTwiddling.h"

typedef struct {
   Entity *e;
   StringView name;
   Recti area;
   vec(StringPtr) *queue;
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

   box.e = entityCreate(self->view->entitySystem);
   {
      TextComponent tc = { .bg = 0,.fg = 15,.lines = vecCreate(TextLine)(&textLineDestroy) };
      int y;

      for (y = area.top; y < area.bottom; ++y) {
         vecPushBack(TextLine)(tc.lines, &(TextLine){area.left, y, stringCreate("")});
      }

      COMPONENT_ADD(box.e, LayerComponent, LayerUI);
      COMPONENT_ADD(box.e, RenderedUIComponent, 0);
      entityAdd(TextComponent)(box.e, &tc);
      entityUpdate(box.e);
   }

   box.area = area;
   box.name = name;
   box.queue = vecCreate(StringPtr)(&stringPtrDestroy);

   htInsert(TextBox)(self->boxTable, &box);
}
void textBoxManagerPushText(TextBoxManager *self, StringView name, const char *msg) {
   TextBox *found = htFind(TextBox)(self->boxTable, &(TextBox){.name = name});
   if (found) {
      TextComponent *tc = entityGet(TextComponent)(found->e);
      stringSet(vecAt(TextLine)(tc->lines, 0)->text, msg);
   }
}

void textBoxManagerUpdate(TextBoxManager *self) {

}