#include "MusicBox.h"

int main() {
   MusicBox *mbox = musicBoxCreate();

   musicBoxTest(mbox);

   while (!musicBoxIsDone(mbox)) {}

   musicBoxDestroy(mbox);


   return 0;
}