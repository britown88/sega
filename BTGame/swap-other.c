#include <math.h>

#include "segashared\CheckedMemory.h"

#include "Actions.h"
#include "Commands.h"
#include "Managers.h"
#include "CombatRoutines.h"
#include "GridManager.h"
#include "CoreComponents.h"
#include "LogManager.h"
#include "Combat.h"

typedef struct {
   WorldView *view;
   Action *a;
   bool started, paused;
   Entity *placeHolder;
   Int2 dest;
}SwapOtherRoutineData;

static SwapOtherRoutineData *SwapOtherRoutineDataCreate(){
   return checkedCalloc(1, sizeof(SwapOtherRoutineData));
}

static void _SwapOtherRoutineDestroy(SwapOtherRoutineData *self){
   checkedFree(self);
}

static CoroutineStatus _SwapOtherRoutine(SwapOtherRoutineData *data, CoroutineRequest request){
   BTManagers *managers = data->view->managers;
   ActionUserComponent *uc = entityGet(ActionUserComponent)(data->a);
   ActionTargetEntityComponent *tec = entityGet(ActionTargetEntityComponent)(data->a);
   ActionCombatComponent *cc = entityGet(ActionCombatComponent)(data->a);
   
   Entity *e, *target;

   if (!uc || !tec || !cc){
      //shouldnt get here but return done if we dont ahve the right components!      
      return Finished;
   }

   e = uc->user;
   target = tec->target;

   if (request == Pause && !data->paused){

      if (data->started){
         if (entityGet(InterpolationComponent)(e)){
            entityRemove(InterpolationComponent)(e);
            entityUpdate(e);
         }
      }
      data->paused = true;
      return NotFinished;
   }

   if (request == ForceCancel){
      //snap to and finish
      if (data->started){
         InterpolationComponent *ic = entityGet(InterpolationComponent)(e);

         //set new gridpos
         COMPONENT_LOCK(GridComponent, newgc, e, {
            newgc->x = data->dest.x;
            newgc->y = data->dest.y;
         });

         //kill the placeholder
         COMPONENT_ADD(data->placeHolder, DestructionComponent, 0);
         entityUpdate(data->placeHolder);

         //kill the inerpolation and snap the position
         if (ic){
            COMPONENT_LOCK(PositionComponent, ppc, e, {
               ppc->x = ic->destX;
               ppc->y = ic->destY;
            });
            entityRemove(InterpolationComponent)(e);
            entityUpdate(e);
         }
      }

      //all cleaned up for forcecancel, return finished
      return Finished;
   }
   else{
      //not cancelled
      GridComponent *gc = entityGet(GridComponent)(e);
      GridComponent *gc2 = entityGet(GridComponent)(target);

      if (!gc || !gc2){
         //wrong comps
         return Finished;
      }

      if (data->paused){
         //resuming
         data->paused = false;
         if (data->started){
            //re-add in the interp-comp
            Int2 destPos;
            screenPosFromGridXY(data->dest.x, data->dest.y, &destPos.x, &destPos.y);

            COMPONENT_ADD(e, InterpolationComponent,
               .destX = destPos.x,
               .destY = destPos.y,
               .time = 0.1f);
            entityUpdate(e);
         }
      }


      if (entityGet(InterpolationComponent)(e)){
         //still moving, return
         return NotFinished;
      }
      else{
         if (!data->started){
            Int2 destPos;
            //make it happen
            data->dest = (Int2){ gc2->x, gc2->y };
            data->placeHolder = entityCreate(data->view->entitySystem);
            COMPONENT_ADD(data->placeHolder, GridComponent, data->dest.x, data->dest.y);
            entityUpdate(data->placeHolder);

            screenPosFromGridXY(data->dest.x, data->dest.y, &destPos.x, &destPos.y);

            COMPONENT_ADD(e, InterpolationComponent, 
               .destX = destPos.x,
               .destY = destPos.y,
               .time = 0.1f);
            entityUpdate(e);

            data->started = true;
            return NotFinished;
         }
         else{
            //we're done!
            COMPONENT_LOCK(GridComponent, newgc, e, {
               newgc->x = data->dest.x;
               newgc->y = data->dest.y;
            });

            COMPONENT_ADD(data->placeHolder, DestructionComponent, 0);
            entityUpdate(data->placeHolder);
            return Finished;
         }
      }
   }

   return Finished;
}

static Coroutine _buildSwapOther(ClosureData data, WorldView *view, Action *a){
   Coroutine out;
   SwapOtherRoutineData *newData = SwapOtherRoutineDataCreate();
   newData->view = view;
   newData->a = a;
   closureInit(Coroutine)(&out, newData, (CoroutineFunc)&_SwapOtherRoutine, &_SwapOtherRoutineDestroy);
   return out;
}

CombatRoutineGenerator buildSwapOtherAttackRoutine(){
   CombatRoutineGenerator out;
   closureInit(CombatRoutineGenerator)(&out, NULL, (CombatRoutineGeneratorFunc)&_buildSwapOther, NULL);
   return out;
}
