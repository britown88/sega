#include "StatusManager.h"
#include "Managers.h"

static Status *_buildStunStatus(ClosureData *data, WorldView *view){
   Status *out = statusCreateCustom(view->managers->statusManager);
   COMPONENT_ADD(out, StatusNameComponent, stringIntern("stun"));
   COMPONENT_ADD(out, StatusInflictsStunComponent, 0);
   return out;
}

StatusGenerator buildStunStatus(){
   StatusGenerator out = { 0 };

   closureInit(StatusGenerator)(&out, NULL, (StatusGeneratorFunc)&_buildStunStatus, NULL);

   return out;
}