#include "Coroutine.h"

#define ClosureTPart CLOSURE_NAME(Coroutine)
#include "Closure_Impl.h"

#define VectorTPart Coroutine
#include "Vector_Impl.h"

static void _coroutineDestroy(Coroutine *self){
   closureDestroy(Coroutine)(self);
}

static void _coroutineVectorDestroy(vec(Coroutine) *self){
   vecDestroy(Coroutine)(self);
}

static CoroutineStatus _synchronized(vec(Coroutine) *list, bool cancel){
   vec(Coroutine) *deleteList = NULL;
   CoroutineStatus status = Finished;

   vecForEach(Coroutine, /*Coroutine* */c, list, {
      CoroutineStatus status = closureCall(c, cancel);

      if (status == Finished){
         if (!deleteList){
            deleteList = vecCreate(Coroutine)(NULL);
         }

         vecPushBack(Coroutine)(deleteList, c);
      }
      else{
         //this ones not done so we can bail out as unfinished
         break;
      }      
   });

   // remove the deleted ones out of our main list 
   // and destroy the delete list
   if (deleteList){
      vecForEach(Coroutine, /*Coroutine* */c, deleteList, {
         vecRemove(Coroutine)(list, c);
      });
      
      vecDestroy(Coroutine)(deleteList);
   }

   //notfinished if we bailed out, finshed if our list was empty or if all finished
   return status;
}

Coroutine createSynchronizedList(vec(Coroutine) **listOut){
   Coroutine out;
   vec(Coroutine) *list = vecCreate(Coroutine)(&_coroutineDestroy);
   *listOut = list;
   closureInit(Coroutine)(&out, list, (CoroutineFunc)&_synchronized, &_coroutineVectorDestroy);
   return out;
}

typedef struct {
   vec(Coroutine) *list;
   size_t iter;
} ExecutionListData;

static ExecutionListData *_executionDataCreate(){
   ExecutionListData *out = checkedCalloc(1, sizeof(ExecutionListData));
   out->list = vecCreate(Coroutine)(&_coroutineDestroy);
   out->iter = 0;
   return out;
}

static void _executionDataDestroy(ExecutionListData *self){
   vecDestroy(Coroutine)(self->list);
   checkedFree(self);
}

static CoroutineStatus _execution(ExecutionListData *data, bool cancel){
   size_t count = vecSize(Coroutine)(data->list);

   //sanity check to see if we're done before we start
   if (data->iter >= count){
      return Finished;
   }

   if (closureCall(vecAt(Coroutine)(data->list, data->iter), cancel) == Finished){
      ++data->iter;
   }

   //check and see if the list is done
   if (data->iter >= count){
      return Finished;
   }

   return NotFinished;
}

Coroutine createExecutionList(vec(Coroutine) **listOut){
   Coroutine out;
   ExecutionListData *data = _executionDataCreate();
   *listOut = data->list;
   
   closureInit(Coroutine)(&out, data, (CoroutineFunc)&_execution, &_executionDataDestroy);

   return out;
}