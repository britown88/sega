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
#include <string.h>
#include <stdio.h>
#include "segautils/Math.h"

typedef enum {
   Splash = 0,
   Characters,
   Intro,
   Game,
   Credits
}GBStates;

typedef enum {
   GetAngle = 0,
   GetPower,
   Throw
}TurnStates;

typedef struct {
   WorldView *view;

   GBStates state;

   Microseconds time;
   int marqueeOffset;

   TextArea *txt;
   ManagedImage *sun, *gorilla;

   Texture *buildingsTexture;

   Int2 bCoor[30];
   Int2 gPos[2];
   int bCount;

   String *pNames[2];
   int player;
   TurnStates turnState;
   int angle[2], power[2];
   
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

   //mouth
   textureRenderEllipseQB(frame, NULL, x, y + 5, 3, 0, -1.0);
   textureRenderFloodFill(frame, NULL, x, y+5, 0, 0);

   //eyes
   textureRenderEllipseQB(frame, NULL, x-3, y-2, 1, 0, -1.0);
   textureRenderEllipseQB(frame, NULL, x + 3, y - 2, 1, 0, -1.0);
   textureRenderPoint(frame, NULL, x - 3, y - 2, 0);
   textureRenderPoint(frame, NULL, x + 3, y - 2, 0);

}

//porting from gorillas, bcoor is topleft coords of buildings
static Texture *makeCityScape(SplashState *state) {
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

      state->bCoor[curBuilding].x = x;
      state->bCoor[curBuilding].y = bottomLine - bHeight;
      
      //render
      {
         int color = appRand(appGet(), 1, 4) + 4;     

         textureRenderLineRect(buildingFrame, NULL, x - 1, bottomLine - bHeight - 1, x + bWidth + 1+1, bottomLine + 1+1, 0);
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

   state->bCount = curBuilding;
   return buildingFrame;
}
static void placeGorillas(Texture *frame, SplashState *state) {
   int xAdj = 14;
   int yAdj = 30;

   Texture *gorilla = managedImageGetTexture(state->gorilla);
   size_t gWidth = textureGetWidth(gorilla);
   size_t gHeight = textureGetHeight(gorilla);

   int i;

   for (i = 0; i < 2; ++i) {
      int bNum = i == 0 ? appRand(appGet(), 1, 3) + 1 : state->bCount - 1 - appRand(appGet(), 0, 2);
      int bWidth = state->bCoor[bNum].x - state->bCoor[bNum - 1].x;
      state->gPos[i].x = state->bCoor[bNum - 1].x + bWidth / 2 - xAdj;
      state->gPos[i].y = state->bCoor[bNum - 1].y - yAdj + 1;

      textureRenderTexture(frame, NULL, state->gPos[i].x, state->gPos[i].y, gorilla);
   }
}

static void testCity(SplashState *state) {

   if (state->buildingsTexture) {
      textureDestroy(state->buildingsTexture);
   }

   state->buildingsTexture = makeCityScape(state);
   placeGorillas(state->buildingsTexture, state);
}



static void _startGame(SplashState *state) {
   assetsSetPalette(state->view->db, stringIntern("GORILLAS"));
   testCity(state);
   state->state = Game;


   stringSet(state->pNames[0], "Jeff");
   stringSet(state->pNames[1], "Vinny");
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

   //textureRenderEllipseQB(frame, NULL, EGA_RES_WIDTH/2, 25, 12, 14, -1.0);

   //textureRenderEllipse(frame, NULL, 10, 10, 5, 5, 3);
   //textureRenderEllipse(frame, NULL, 100, 100, 50, 75, 4);
   //textureRenderEllipse(frame, NULL, 100, 100, 100, 25, 6);

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
            _startGame(state);
            return;
         }
      }
   }
}

#define BAR_WIDTH (EGA_TEXT_CHAR_WIDTH*20)
#define MAX_POWER 200
#define MS_TO_MAX 2000

static void gbUpdateGame(SplashState *state) {
   if (state->turnState == GetPower) {
      Milliseconds delta = t_u2m(appGetTime(appGet()) - state->time);

      if (delta > MS_TO_MAX) {
         state->power[state->player] = MAX_POWER;
         state->turnState = Throw;
         state->time = appGetTime(appGet());
      }
      else {
         double d = delta / (double)MS_TO_MAX;
         state->power[state->player] = MAX_POWER * d;
      }
   }
   else if (state->turnState == Throw) {
      if (t_u2m(appGetTime(appGet()) - state->time) > 1000) {
         state->turnState = GetAngle;
         state->player = !state->player;
         state->time = appGetTime(appGet());
      }
   }

}

static void _drawGameUI(SplashState *state, Texture *frame) {
   Font *UIFont = fontFactoryGetFont(state->view->fontFactory, 0, 9);
   Font *UIGreenFont = fontFactoryGetFont(state->view->fontFactory, 0, 10);
   //player names
   textureRenderText(frame, c_str(state->pNames[0]), 0, 0, UIFont);
   textureRenderText(frame, c_str(state->pNames[1]), EGA_TEXT_RES_WIDTH - 2 - stringLen(state->pNames[1]), 0, UIFont);

   if (state->player == 0) {
      char buff[10] = { 0 };
      sprintf(buff, "%i", state->angle[state->player]);

      textureRenderText(frame, "Angle [ \x18 \x19 ]:", 0, 1, UIFont);
      textureRenderText(frame, buff, 15, 1, UIGreenFont);

      sprintf(buff, "%i", state->power[state->player]);
      textureRenderText(frame, "Velocity [ Hold SPACE ]:", 0, 2, UIFont);
      textureRenderText(frame, buff, 25, 2, UIGreenFont);
   }
   else {
      char buff[10] = { 0 };
      sprintf(buff, "%i", state->angle[state->player]);
      int angleLen = 15 + strlen(buff) + 2;

      textureRenderText(frame, "Angle [ \x18 \x19 ]:", EGA_TEXT_RES_WIDTH - angleLen, 1, UIFont);
      textureRenderText(frame, buff, EGA_TEXT_RES_WIDTH - strlen(buff) - 2, 1, UIGreenFont);

      sprintf(buff, "%i", state->power[state->player]);
      angleLen = 25 + strlen(buff) + 2;

      textureRenderText(frame, "Velocity [ Hold SPACE ]:", EGA_TEXT_RES_WIDTH - angleLen, 2, UIFont);
      textureRenderText(frame, buff, EGA_TEXT_RES_WIDTH - strlen(buff) - 2, 2, UIGreenFont);
   }
}

static void _drawAngleArrow(SplashState *state, Texture *frame) {
   Int2 pos1 = state->gPos[state->player];
   int arrowLen = 50;

   Int2 pos2 = { 0 };
   float rad = state->angle[state->player]*(PI / 180.0f);

   pos1.x += 14;
   pos1.y += 15;

   pos2.x = state->player == 0 ? (cosf(rad)*arrowLen + pos1.x) : (pos1.x - cosf(rad)*arrowLen);
   pos2.y = pos1.y - sinf(rad)*arrowLen;

   textureRenderLine(frame, NULL, pos1.x, pos1.y, pos2.x, pos2.y, 3);

}

static void _drawPowerBar(SplashState *state, Texture *frame) {
   int i;
   Font *UIFont = fontFactoryGetFont(state->view->fontFactory, 0, 9);
   Int2 pos = { EGA_TEXT_CHAR_WIDTH*6, EGA_TEXT_CHAR_HEIGHT * (EGA_TEXT_RES_HEIGHT - 1) };

   if (state->player == 1) {
      pos.x = (EGA_TEXT_RES_WIDTH - 2) * EGA_TEXT_CHAR_WIDTH - BAR_WIDTH;
   }

   int powerWidth = state->power[state->player];

   powerWidth = (powerWidth / (float)MAX_POWER) * BAR_WIDTH;

   if (state->player == 0) {
      textureRenderText(frame, "POWER:", 0, EGA_TEXT_RES_HEIGHT - 1, UIFont);
   }
   else {
      textureRenderText(frame, "POWER:", EGA_TEXT_RES_WIDTH - (BAR_WIDTH/EGA_TEXT_CHAR_WIDTH) - 6-2, EGA_TEXT_RES_HEIGHT - 1, UIFont);
   }
   

   textureRenderRect(frame, NULL, pos.x, pos.y, pos.x + BAR_WIDTH, pos.y + EGA_TEXT_CHAR_HEIGHT, 6);
   textureRenderRect(frame, NULL, pos.x, pos.y, pos.x + powerWidth, pos.y + EGA_TEXT_CHAR_HEIGHT, 10);

   for (i = 0; i < 4; ++i) {
      int lineX = pos.x + ((BAR_WIDTH / 4.0f) * (i + 1));
      textureRenderLine(frame, NULL, lineX, pos.y, lineX, pos.y + EGA_TEXT_CHAR_HEIGHT, 4);
   }
   
}

static void gbRenderGame(SplashState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   textureClear(frame, NULL, 0);

   //do buildings/gorillas
   if (state->buildingsTexture) {
      textureRenderTexture(frame, NULL, 0, 0, state->buildingsTexture);
   }

   //doSun(frame);

   //dosun
   Texture *sun = managedImageGetTexture(state->sun);
   textureRenderTexture(frame, NULL, (EGA_RES_WIDTH - textureGetWidth(sun)) / 2, 25 - textureGetHeight(sun) / 2, sun);

   //do UI
   _drawGameUI(state, frame);

   if (state->turnState != Throw) {
      _drawAngleArrow(state, frame);
      _drawPowerBar(state, frame);
   }
   
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
         case SegaKey_F1:
            testCity(state);
            break;
         case SegaKey_Space:
            if (state->turnState == GetPower) {
               state->time = appGetTime(appGet());
               state->turnState = Throw;
            }
            break;
         }         
      }
      if (e.action == SegaKey_Pressed) {
         if (e.key == SegaKey_Space && state->turnState == GetAngle) {
            state->power[state->player] = 0;
            state->time = appGetTime(appGet());
            state->turnState = GetPower;
         }
      }
   }

   if (state->turnState == GetAngle) {
      int *angle = state->angle + state->player;
      if (keyboardIsDown(k, SegaKey_Up)) {
         *angle = (*angle + 1) % 360;         
      }
      if (keyboardIsDown(k, SegaKey_Down)) {
         *angle = (*angle - 1) % 360;
         if (*angle < 0) {
            *angle += 360;
         }
      }
   }   
}





static void _splashStateCreate(SplashState *state) {
   int i = 0;
   for (i = 0; i < 2; ++i) {
      state->pNames[i] = stringCreate("");
   }

   state->txt = textAreaCreate(0, 4, EGA_TEXT_RES_WIDTH, 9);   
   textAreaSetJustify(state->txt, TextAreaJustify_Center);

   state->sun = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gorilla-sun"));
   state->gorilla = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gorilla"));


}
static void _splashStateDestroy(SplashState *self) {
   int i = 0;
   for (i = 0; i < 2; ++i) {
      stringDestroy(self->pNames[i]);
   }

   if (self->buildingsTexture) {
      textureDestroy(self->buildingsTexture);
   }
   
   managedImageDestroy(self->sun);
   managedImageDestroy(self->gorilla);
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