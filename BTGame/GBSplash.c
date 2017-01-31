#include "WorldView.h"
#include "Managers.h"
#include "LightGrid.h"
#include "Console.h"
#include "GameState.h"
#include "GameHelpers.h"
#include "ImageLibrary.h"
#include "MapEditor.h"
#include "DB.h"
#include "Lua.h"

#include "SEGA\Input.h"
#include "SEGA\App.h"
#include "GameClock.h"
#include "TextArea.h"

#include "segashared\CheckedMemory.h"

#include "ImageLibrary.h"
#include "Sprites.h"
#include "AssetHelpers.h"

#include <math.h>

typedef enum {
   Splash = 0,
   Characters,
   Intro,
   Game,
   Credits
}GBStates;

typedef struct {
   WorldView *view;

   GBStates state;

   Microseconds time;
   int marqueeOffset;

   TextArea *txt;
   ManagedImage *gblogo;

   Texture *buildingsTexture;
}SplashState;

static int CINT(double d) { return round(d); }

static void doSun(Texture *frame) {
   int x = EGA_RES_WIDTH / 2;
   int y = 25;

   //clear old sun
   textureRenderRect(frame, NULL, x - 22, y - 18, x + 23, y + 19, 1);

   //body
   textureRenderEllipseQB(frame, NULL, x, y, 12, 14, -1.0);
   textureRenderFloodFill(frame, NULL, x, y, 14, 14);

   //rays
   textureRenderLine(frame, NULL, x - 20, y, x + 20, y, 14);
   textureRenderLine(frame, NULL, x, y - 15, x, y + 15, 14);

   textureRenderLine(frame, NULL, x-15, y-10, x+15, y+10, 14);
   textureRenderLine(frame, NULL, x-15, y+10, x+15, y-10, 14);

   textureRenderLine(frame, NULL, x - 8, y-13, x + 8, y+13, 14);
   textureRenderLine(frame, NULL, x - 8, y+13, x + 8, y-13, 14);

   textureRenderLine(frame, NULL, x - 18, y-5, x + 18, y+5, 14);
   textureRenderLine(frame, NULL, x - 18, y+5, x + 18, y-5, 14);

   //eyes
   textureRenderEllipseQB(frame, NULL, x-3, y-2, 1, 0, -1.0);
   textureRenderEllipseQB(frame, NULL, x + 3, y - 2, 1, 0, -1.0);
   textureRenderPoint(frame, NULL, x - 3, y - 2, 0);
   textureRenderPoint(frame, NULL, x + 3, y - 2, 0);

}

//porting from gorillas, bcoor is topleft coords of buildings
static Texture *makeCityScape(Int2 *bcoor) {
   Texture *buildingFrame = textureCreate(EGA_RES_WIDTH, EGA_RES_HEIGHT);

   int x = 2;   
   int slope = appRand(appGet(), 0, 6);
   int newHt = slope < 4 ? 15 : 130;

   int screenWidth = EGA_RES_WIDTH;
   int bottomLine = 335;
   int htInc = 10;
   int defBWidth = 37;
   int randomHeight = 120;
   int wWidth = 3;
   int wHeight = 6;
   int wDifV = 15;
   int wDifh = 10;
   int curBuilding = 0;
   int GHeight = 25;//?
   int maxHeight = 0;   

   do {
      int bWidth = 0, bHeight = 0;

      switch (slope) {
      case 0: newHt += htInc; break;
      case 1: newHt -= htInc; break;
      case 2: case 3: case 4:
         if (x > screenWidth / 2) {
            newHt -= 2 * htInc;
         }
         else {
            newHt += 2 * htInc;
         }
         break;
      case 5:
         if (x > screenWidth / 2) {
            newHt += 2 * htInc;
         }
         else {
            newHt -= 2 * htInc;
         }
 
         break;
      }

      bWidth = appRand(appGet(), 0, defBWidth) + 1 + defBWidth;
      if (x + bWidth > screenWidth) {
         bWidth = screenWidth - x - 2;
      }

      bHeight = appRand(appGet(), 0, randomHeight) + 1 + newHt;
      if (bHeight < htInc) {
         bHeight = htInc;
      }

      if (bottomLine - bHeight <= maxHeight + GHeight) {
         bHeight = maxHeight + GHeight - 5;
      }

      bcoor[curBuilding].x = x;
      bcoor[curBuilding].y = bottomLine - bHeight;
      
      //render
      {
         int color = appRand(appGet(), 0, 3);
         color = color == 0 ? 3 : (color == 1 ? 4 : 7);         

         textureRenderLineRect(buildingFrame, NULL, x - 1, bottomLine - bHeight - 1, x + bWidth + 1+1, bottomLine + 1+1, 1);
         textureRenderRect(buildingFrame, NULL, x, bottomLine - bHeight, x + bWidth+1, bottomLine+1, color);

         int c = x + 3;
         do {
            int i;
            for (i = bHeight - 3; i >= 7; i -= wDifV) {
               int winColor = appRand(appGet(), 0, 4) ? 14 : 8;
               textureRenderRect(buildingFrame, NULL, c, bottomLine - i, c+wWidth+1, bottomLine - i + wHeight+1, winColor);
            }
            c += wDifh;
         } while (c < x + bWidth - 3);
      }

      x += bWidth + 2;
      ++curBuilding;
   } while (x <= screenWidth - htInc);

   return buildingFrame;
}
static void testCity(SplashState *state) {
   Int2 buildings[30];
   if (state->buildingsTexture) {
      textureDestroy(state->buildingsTexture);
   }

   state->buildingsTexture = makeCityScape(buildings);
}

static void gbUpdateSplash(SplashState *state) {
   static const int interval = 100000;
   Microseconds t = appGetTime(appGet());

   if (t - state->time > interval) {
      state->time += interval;
      state->marqueeOffset = (state->marqueeOffset + 1) % 5;
   }
}
static void _renderBorder(SplashState *state, Texture *frame) {
   Font *font = fontFactoryGetFont(state->view->fontFactory, 0, 4);
   int i = 0;   

   for (i = 0; i < EGA_TEXT_RES_WIDTH; ++i) {
      if (i % 5 == state->marqueeOffset) {
         textureRenderTextSingleChar(frame, '*', i, 0, font, false);
      }
      if (i % 5 == 4 - state->marqueeOffset) {
         textureRenderTextSingleChar(frame, '*', i, EGA_TEXT_RES_HEIGHT - 1, font, false);
      }
   }

   for (i = 1; i < EGA_TEXT_RES_HEIGHT - 1; ++i) {

      if (i % 5 == state->marqueeOffset) {
         textureRenderTextSingleChar(frame, '*', EGA_TEXT_RES_WIDTH - 1, i, font, false);
      }

      if (i % 5 == 4 - state->marqueeOffset) {
         textureRenderTextSingleChar(frame, '*', 0, i, font, false);
      }
   }

}
static void gbRenderSplash(SplashState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   textureClear(frame, NULL, 0);
   _renderBorder(state, frame);

   textAreaRender(state->txt, state->view, frame);

   textureRenderEllipse(frame, NULL, 10, 10, 5, 5, 3);
   textureRenderEllipse(frame, NULL, 100, 100, 50, 75, 4);
   textureRenderEllipse(frame, NULL, 100, 100, 100, 25, 6);

   //textureRenderFloodFill(frame, NULL, 200, 200, 6, 6);
}
static void gbHandleKeyboardSplash(SplashState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         default:
            testCity(state);
            state->state = Game;
            return;
         }
      }
   }
}

static void gbUpdateGame(SplashState *state) {
}
static void gbRenderGame(SplashState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   textureClear(frame, NULL, 1);

   if (state->buildingsTexture) {
      textureRenderTexture(frame, NULL, 0, 0, state->buildingsTexture);
   }

   doSun(frame);
}



static void gbHandleKeyboardGame(SplashState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {
         switch (e.key) {
         case SegaKey_Escape:
            appQuit(appGet());
            break;
         default:
            testCity(state);
            break;
         }
         
      }
   }
}





static void _splashStateCreate(SplashState *state) {
   state->txt = textAreaCreate(0, 4, EGA_TEXT_RES_WIDTH, 9);   
   textAreaSetJustify(state->txt, TextAreaJustify_Center);
   state->gblogo = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gb_logo"));
}
static void _splashStateDestroy(SplashState *self) {

   if (self->buildingsTexture) {
      textureDestroy(self->buildingsTexture);
   }
   
   managedImageDestroy(self->gblogo);
   textAreaDestroy(self->txt);
   checkedFree(self);
}

static void _splashUpdate(SplashState*, GameStateUpdate*);
static void _splashHandleInput(SplashState*, GameStateHandleInput*);
static void _splashRender(SplashState*, GameStateRender*);
static void _splashEnter(SplashState*, StateEnter*);
static void _splashExit(SplashState*, StateExit*);

static void _splash(SplashState *state, Type *t, Message m) {
   if (t == GetRTTI(GameStateUpdate)) { _splashUpdate(state, m); }
   else if (t == GetRTTI(GameStateHandleInput)) { _splashHandleInput(state, m); }
   else if (t == GetRTTI(GameStateRender)) { _splashRender(state, m); }
   else if (t == GetRTTI(StateEnter)) { _splashEnter(state, m); }
   else if (t == GetRTTI(StateExit)) { _splashExit(state, m); }
}

void _splashEnter(SplashState *state, StateEnter *m) {
   WorldView *view = state->view;

   assetsSetPalette(view->db, stringIntern("CGA"));

   state->time = appGetTime(appGet());
   state->marqueeOffset = 0;

   textAreaSetText(state->txt, 
      "Q B a s i c   B O M B I L L A S\n\n"
      "[c=0,7]Copyright (C) Macrofurst Corporation 1990\n\n"
      "Your mission is to hit the opponent with the exploding\n"
      "objects by varying the angle and power of your throw, taking\n"
      "into account wind speed, gravity, and the city skyline.\n"
      "The wind speed is shown by a directional arrow at the bottom\n"
      "of the playing field, its length relative to its strength.[/c]");
}
void _splashExit(SplashState *state, StateExit *m) {
   WorldView *view = state->view;

}

void _splashUpdate(SplashState *state, GameStateUpdate *m) {
   WorldView *view = state->view;

   switch (state->state) {
   case Splash:
      gbUpdateSplash(state);
      break;
   case Game:
      gbUpdateGame(state);
      break;
   }

}


void _splashHandleInput(SplashState *state, GameStateHandleInput *m) {
   switch (state->state) {
   case Splash:
      gbHandleKeyboardSplash(state);
      break;
   case Game:
      gbHandleKeyboardGame(state);
      break;
   }
}

void _splashRender(SplashState *state, GameStateRender *m) {
   switch (state->state) {
   case Splash:
      gbRenderSplash(state, m);
      break;
   case Game:
      gbRenderGame(state, m);
      break;
   }
}

StateClosure gameStateCreateGBSplash(WorldView *view) {
   StateClosure out;
   SplashState *state = checkedCalloc(1, sizeof(SplashState));
   state->view = view;

   _splashStateCreate(state);

   closureInit(StateClosure)(&out, state, (StateClosureFunc)&_splash, &_splashStateDestroy);
   return out;
}