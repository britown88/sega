#include "Managers.h"
#include "segashared\CheckedMemory.h"
#include "Entities\Entities.h"
#include "MeshRendering.h"
#include "CoreComponents.h"
#include "SEGA\App.h"
#include "SEGA\Input.h"
#include "LogManager.h"
#include "segashared\Strings.h"
#include "WorldView.h"

#define LOG_LINE_LEN 20
#define LOG_LINE_COUNT 14
#define LOG_X 2
#define LOG_Y 9
#define LOG_COLOR_FG 0
#define LOG_COLOR_BG 7

typedef struct {
   String *line;
}LogEntry;

static void _logEntryDestroy(LogEntry *self){
   stringDestroy(self->line);
}

#define VectorT LogEntry
#include "segautils\Vector_Create.h"

struct LogManager_t{
   Manager m;
   WorldView *view;

   vec(EntityPtr) *logLines;
   vec(LogEntry) *log;
   size_t logPosition;
};

ImplManagerVTable(LogManager)

static Entity *_createLine(EntitySystem *system, int line){
   Entity *e = entityCreate(system);
   COMPONENT_ADD(e, LayerComponent, LayerUI);
   COMPONENT_ADD(e, TextComponent, 
                     .text = stringCreate(""),
                     .x = LOG_X, .y = LOG_Y + line,
                     .fg = LOG_COLOR_FG, .bg = LOG_COLOR_BG);
   entityUpdate(e);
   return e;
}

static void _createLines(LogManager *self){
   int i = 0;
   for (i = 0; i < LOG_LINE_COUNT; ++i){
      Entity *e = _createLine(self->view->entitySystem, i);
      vecPushBack(EntityPtr)(self->logLines, &e);
   }
}

LogManager *createLogManager(WorldView *view){
   LogManager *out = checkedCalloc(1, sizeof(LogManager));
   out->view = view;
   out->m.vTable = CreateManagerVTable(LogManager);

   out->logLines = vecCreate(EntityPtr)(NULL);
   out->log = vecCreate(LogEntry)(&_logEntryDestroy);

   _createLines(out);

   return out;
}

void _destroy(LogManager *self){

   vecDestroy(EntityPtr)(self->logLines);
   vecDestroy(LogEntry)(self->log);

   checkedFree(self);
}
void _onDestroy(LogManager *self, Entity *e){}
void _onUpdate(LogManager *self, Entity *e){}

void logManagerUpdate(LogManager *self){
   int size = vecSize(LogEntry)(self->log);
   if (self->logPosition < (size_t)size){
      int i = 0;
      StringView emptyString = stringIntern("");
      vecForEach(EntityPtr, e, self->logLines, {
         stringClear(entityGet(TextComponent)(*e)->text);
      });

      self->logPosition = size--;

      for (i = LOG_LINE_COUNT - 1; i >= 0 && size >= 0; --i, --size){
         Entity *e = *vecAt(EntityPtr)(self->logLines, i);
         TextComponent *tc = entityGet(TextComponent)(e);
         String *line = vecAt(LogEntry)(self->log, size)->line;
         stringConcat(tc->text, c_str(line));
      }
   }
}

void logManagerPushString(LogManager *self, String *text){
   vecPushBack(LogEntry)(self->log, &(LogEntry){text});
}