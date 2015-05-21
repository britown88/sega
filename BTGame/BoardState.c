#include "WorldView.h"
#include "Managers.h"
#include "SelectionManager.h"
#include "GridManager.h"
#include "Entities\Entities.h"
#include "ImageLibrary.h"
#include "segalib\EGA.h"

#include "GameState.h"
#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "segashared\CheckedMemory.h"

#include "Commands.h"
#include "CoreComponents.h"
#include "segashared\Strings.h"
#include "LogManager.h"
#include "GameClock.h"
#include "Combat.h"

typedef struct {
   WorldView *view;
   bool paused;
}BoardState;

static void _boardStateDestroy(BoardState *self){
   checkedFree(self);
}

static void _boardUpdate(BoardState*, GameStateUpdate*);
static void _boardHandleInput(BoardState*, GameStateHandleInput*);
static void _boardRender(BoardState*, GameStateRender*);

static void _board(BoardState *state, Type *t, Message m){
   if (t == GetRTTI(GameStateUpdate)){ _boardUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)){ _boardHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)){ _boardRender(state, m); }
}

void _boardUpdate(BoardState *state, GameStateUpdate *m){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Int2 mousePos = mouseGetPosition(mouse);

   cursorManagerUpdate(managers->cursorManager, mousePos.x, mousePos.y);
   interpolationManagerUpdate(managers->interpolationManager);
   diceManagerUpdate(managers->diceManager);

   if (!state->paused){
      commandManagerUpdate(managers->commandManager);
   }

   combatManagerUpdate(managers->combatManager);
   AIManagerUpdate(managers->AIManager);

   logManagerUpdate(managers->logManager);
   destructionManagerUpdate(managers->destructionManager);
}

static bool special = false;

static void _handleKeyboard(BoardState *state){
   BTManagers *managers = state->view->managers;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };
   while (keyboardPopEvent(k, &e)){
      switch (e.key){
      case (SegaKey_1) :
         if (e.action == SegaKey_Released){
            special = true;
         }
                            break;
      case (SegaKey_Escape) :
         if (e.action == SegaKey_Released){
            appQuit(appGet());
         }
                            break;
      case (SegaKey_Space) :
         if (e.action == SegaKey_Released){
            if (state->paused){
               state->paused = false;
               gameClockResume(state->view->gameClock);
            }
            else{
               state->paused = true;
               gameClockPause(state->view->gameClock);
            }
         }
                           break;
      }

   }
}

static void _handleMouse(BoardState *state){
   BTManagers *managers = state->view->managers;
   Mouse *mouse = appGetMouse(appGet());
   Keyboard *k = appGetKeyboard(appGet());
   MouseEvent event = { 0 };
   while (mousePopEvent(mouse, &event)){

      switch (event.action){
      case SegaMouse_Pressed:
         if (event.button == SegaMouseBtn_Left){
            cursorManagerStartDrag(managers->cursorManager, event.pos.x, event.pos.y);
         }

         break;
      case SegaMouse_Released:
         if (event.button == SegaMouseBtn_Left){
            Recti mouseArea = cursorManagerEndDrag(managers->cursorManager, event.pos.x, event.pos.y);

            selectionManagerSelect(managers->selectionManager,
            { scArea, .box = mouseArea },
            { scTeam, .teamID = 0 });
         }
         else if (event.button == SegaMouseBtn_Right){
            bool shift = keyboardIsDown(k, SegaKey_LeftShift) || keyboardIsDown(k, SegaKey_RightShift);
            vec(EntityPtr) *selectedEntities = selectionManagerGetSelected(managers->selectionManager);
            vec(EntityPtr) *clickedEntities;
            size_t gridIndex = gridIndexFromScreenXY(event.pos.x, event.pos.y);

            selectionManagerGetEntities(managers->selectionManager, clickedEntities,
            { scArea, .box = (Recti){ event.pos.x, event.pos.y, event.pos.x, event.pos.y } },
            { scTeam, .teamID = 1 });

            if (!vecIsEmpty(EntityPtr)(clickedEntities)){
               logManagerPushMessage(managers->logManager, "Clicked an Entity");
               vecForEach(EntityPtr, e, selectedEntities, {
                  if (!shift){
                     entityCancelCommands(*e);
                  }

                  entityPushCommand(*e, createActionCombatSlot(managers->commandManager, special ? 1 : 0, *vecAt(EntityPtr)(clickedEntities, 0)));
               });
               special = false;
            }
            else{
               selectionManagerGetEntities(managers->selectionManager, clickedEntities,
               { scArea, .box = (Recti){ event.pos.x, event.pos.y, event.pos.x, event.pos.y } },
               { scTeam, .teamID = 0 });

               if (!vecIsEmpty(EntityPtr)(clickedEntities)){
                  vecForEach(EntityPtr, e, selectedEntities, {
                     if (!shift){
                        entityCancelCommands(*e);
                     }

                     entityPushCommand(*e, createActionGridTarget(managers->commandManager, *vecAt(EntityPtr)(clickedEntities, 0), 0.0f));
                  });
               }
               else if (gridIndex < INF){
                  int gx, gy;
                  logManagerPushMessage(managers->logManager, "Clicked a Position");
                  gridXYFromIndex(gridIndex, &gx, &gy);
                  vecForEach(EntityPtr, e, selectedEntities, {
                     if (!shift){
                        entityCancelCommands(*e);
                     }

                     entityPushCommand(*e, createActionGridPosition(managers->commandManager, gx, gy));
                  });
               }
            }
         }
         break;
      }

      if (event.action == SegaMouse_Scrolled){
         //size += e.pos.y;
      }

   }
}

void _boardHandleInput(BoardState *state, GameStateHandleInput *m){
   _handleKeyboard(state);
   _handleMouse(state);
}

void _boardRender(BoardState *state, GameStateRender *m){
   renderManagerRender(state->view->managers->renderManager, m->frame);
}

static byte foo = 0;
static void _createTestEntity(EntitySystem *system, int x, int y, bool AI){
   Entity *e = entityCreate(system);

   COMPONENT_ADD(e, PositionComponent, 0, 0);
   COMPONENT_ADD(e, ImageComponent, stringIntern(AI ? "assets/img/dota/ursa.ega" : "assets/img/dota/venge.ega"));

   COMPONENT_ADD(e, LayerComponent, LayerTokens);;
   COMPONENT_ADD(e, GridComponent, .x = x, .y = y);
   COMPONENT_ADD(e, SizeComponent, GRID_RES_SIZE, GRID_RES_SIZE);
   COMPONENT_ADD(e, TeamComponent, AI ? 1 : 0);
   COMPONENT_ADD(e, CombatSlotsComponent, .slots = { stringIntern(!AI ? "bow" : "melee"), stringIntern("swap") });
   //if (true){
      COMPONENT_ADD(e, AIComponent, 0);
   //}
   //COMPONENT_ADD(e, WanderComponent, 1);

   COMPONENT_ADD(e, StatsComponent, .strength = 25, .agility = 25, .intelligence = 25);

   COMPONENT_LOCK(StatsComponent, sc, e, {
      sc->HP = entityGetMaxHP(e);
   });

   entityUpdate(e);
}

StateClosure gameStateCreateBoard(WorldView *view){
   StateClosure out;
   BoardState *state = checkedCalloc(1, sizeof(BoardState));

   int i;

   state->view = view;
   state->paused = false;

   for (i = 0; i < 12; ++i){
      _createTestEntity(view->entitySystem, i, 0, false);
   }

   for (i = 0; i < 12; ++i){
      _createTestEntity(view->entitySystem, 4 + i, 10, true);
   }  


   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_board, &_boardStateDestroy);

   return out;
}