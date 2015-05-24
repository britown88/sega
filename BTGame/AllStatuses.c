#include "StatusManager.h"

void buildAllStatuses(StatusLibrary *self){
   statusLibraryAdd(self, stringIntern("stun"), buildStunStatus());

}