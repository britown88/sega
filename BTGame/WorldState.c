#include "WorldView.h"
#include "Managers.h"
#include "CoreComponents.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"

#include "Entities\Entities.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"

#include "segashared\CheckedMemory.h"

static void _boardStateDestroy(WorldState *self){
   checkedFree(self);
}

static void _boardUpdate(WorldState*, GameStateUpdate*);
static void _boardHandleInput(WorldState*, GameStateHandleInput*);
static void _boardRender(WorldState*, GameStateRender*);

static void _board(WorldState *state, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _boardUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _boardHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)){ _boardRender(state, m); }
}


void _boardUpdate(WorldState *state, GameStateUpdate *m){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
   interpolationManagerUpdate(managers->interpolationManager);
   waitManagerUpdate(managers->waitManager);
   gridMovementManagerUpdate(managers->gridMovementManager);
   pcManagerUpdate(managers->pcManager);
   textBoxManagerUpdate(managers->textBoxManager);
   actorManagerUpdate(managers->actorManager);

   if (consoleGetEnabled(state->view->console)) {
      consoleUpdate(state->view->console);
   }
}



void _boardHandleInput(WorldState *state, GameStateHandleInput *m){

   if (consoleGetEnabled(state->view->console)) {
      worldStateHandleKeyboardConsole(state);
      worldStateHandleMouseConsole(state);
   }
   else {
      worldStateHandleKeyboard(state);
      worldStateHandleMouse(state);
   }   
}

void _boardRender(WorldState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static void _addActor(WorldState *state, int x, int y, int imgX, int imgY) {
   Tile *t = gridManagerTileAtXY(state->view->managers->gridManager, x, y);
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, SizeComponent, 14, 14);
   COMPONENT_ADD(e, RectangleComponent, 0);

   COMPONENT_ADD(e, ImageComponent, .filename = stringIntern("assets/img/tiles.ega"), .partial = true, .x = imgX, .y = imgY, .width = 14, .height = 14);
   COMPONENT_ADD(e, LayerComponent, LayerGrid);
   COMPONENT_ADD(e, InViewComponent, 0);
   COMPONENT_ADD(e, GridComponent, x, y);
   COMPONENT_ADD(e, LightComponent, .radius = 0, .centerLevel = 0, .fadeWidth = 0);
   COMPONENT_ADD(e, ActorComponent, .moveTime = DEFAULT_MOVE_SPEED, .moveDelay = DEFAULT_MOVE_DELAY);

   if (t) {
      t->schema = 3;
      t->collision = 0;
   }

   entityUpdate(e);
}

static void _addTestEntities(WorldState *state) {
   int i;
   Entity *e = entityCreate(state->view->entitySystem);
   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern("assets/img/bg.ega"));
   COMPONENT_ADD(e, LayerComponent, LayerBackground);
   COMPONENT_ADD(e, RenderedUIComponent, 0);
   entityUpdate(e);

   {
      StringView boxName = stringIntern("smallbox");
      textBoxManagerCreateTextBox(state->view->managers->textBoxManager, boxName, (Recti) { 15, 22, 38, 24 });
      textBoxManagerPushText(state->view->managers->textBoxManager, boxName, "You are likely to be eaten by a [c=0,13]grue[/c].");
      textBoxManagerShowTextArea(state->view->managers->textBoxManager, boxName);
      
   }

   for (i = 0; i < 15; ++i) {
      int x = appRand(appGet(), 0, 21);
      int y = appRand(appGet(), 0, 11);
      int sprite = appRand(appGet(), 0, 3);

      _addActor(state, x, y, 70 + (sprite*14), 28);
   }

}

static void _enterState(WorldState *state) {
   appLoadPalette(appGet(), "assets/pal/default.pal");
   cursorManagerCreateCursor(state->view->managers->cursorManager);
   pcManagerCreatePC(state->view->managers->pcManager);
   verbManagerCreateVerbs(state->view->managers->verbManager);
   

   gridManagerSetAmbientLight(state->view->managers->gridManager, STARTING_AMBIENT_LEVEL);

   _addTestEntities(state);

   //_testWordWrap(state);
}

StateClosure gameStateCreateWorld(WorldView *view){
   StateClosure out;
   WorldState *state = checkedCalloc(1, sizeof(WorldState));
   state->view = view;

   _enterState(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}