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
   Throw,
   Explode,
   Win
}TurnStates;

typedef struct {
   float initXVel, initYVel;
   int startXPos, startYPos;
   float t;
   bool onScreen, impact;
}PlotData;

typedef enum
{
   NoArms = 0,
   LeftArm = 1,
   RightArm = 2
}Arms;

typedef enum {
   Drew = 3,
   Brad = 4,
   Jeff = 5,
   Jason = 6,
   Dan = 7,
   Vinny = 8,
   Alex = 9,
   Jeff2 = 10
}Duder;


typedef struct {
   WorldView *view;

   GBStates state;

   Microseconds time;
   int marqueeOffset;

   TextArea *txt;
   ManagedImage *sun, *sun2, *gorilla, *bomb;

   Texture *buildingsTexture;

   Int2 bCoor[30];
   Int2 gPos[2];
   int bCount;

   String *pNames[2];
   int player;
   TurnStates turnState;
   int angle[2], power[2];

   int wind;
   float gravity;

   PlotData plotData;
   Int2 bombPos;
   float expC;
   bool explosionOut;
   bool expRadius;
   bool expColor;

   bool kill;
   bool sunShock;

   Duder duders[2];

   int selectSection, selectOption;
   
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

static void _setWind(SplashState *state) {
   //set wind
   state->wind = appRand(appGet(), 1, 11) - 5;
   if (appRand(appGet(), 0, 3) == 0) {
      state->wind += appRand(appGet(), 1, 11);
   }
   else {
      state->wind -= appRand(appGet(), 1, 11);
   }
}

static void _drawGorilla(SplashState *state, Texture *frame, int x, int y, Arms arms, Duder duder) {
   static int gWidth = 28;
   static int gHeight = 31;

   textureRenderRect(frame, NULL, x, y, x + gWidth, y + gHeight, 0);

   Texture *gorilla = managedImageGetTexture(state->gorilla);
   textureRenderTexturePartial(frame, NULL, x, y, gorilla, arms*gWidth, 0, gWidth, gHeight);
   textureRenderTexturePartial(frame, NULL, x, y, gorilla, duder*gWidth, 0, gWidth, gHeight);
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
   int GHeight = 100;//?
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
      state->gPos[i].y = state->bCoor[bNum - 1].y - yAdj - 1;

      _drawGorilla(state, frame, state->gPos[i].x, state->gPos[i].y, NoArms, state->duders[i]);
   }
}

static void refreshGorilla(SplashState *state, Arms arms) {
   _drawGorilla(state, state->buildingsTexture, state->gPos[state->player].x, state->gPos[state->player].y, arms, state->duders[state->player]);
}

static void buildCity(SplashState *state) {

   if (state->buildingsTexture) {
      textureDestroy(state->buildingsTexture);
   }

   state->buildingsTexture = makeCityScape(state);
   placeGorillas(state->buildingsTexture, state);

   _setWind(state);
}

static void _initCharSelect(SplashState *state) {
   state->state = Characters;

   textAreaSetText(state->txt, 
      "\n\n"
      "[c=0,3]BEST COAST\n\n\n\n\n\n\n\n"
      "BEAST COAST\n\n\n\n\n\n\n\n"
      "GRAVITY[/c]");

   state->selectOption = state->selectSection = 0;
}

static void _startGame(SplashState *state) {
   assetsSetPalette(state->view->db, stringIntern("GORILLAS"));

   stringSet(state->pNames[0], "Drew");
   stringSet(state->pNames[1], "Brad");

   state->duders[0] = Drew;
   state->duders[1] = Brad;

   state->gravity = 9.8;


   state->player = 0;
   state->turnState = GetAngle;
   state->angle[0] = state->angle[1] = state->power[0] = state->power[1] = 0;

   state->kill = false;   
   
   _initCharSelect(state);
}

static void _playGame(SplashState *state) {
   state->state = Game;
   buildCity(state);
}


static void updateExplosion(SplashState *state) {

   float radius = state->expRadius;
   float inc = 0.5f;

   if (state->explosionOut) {
      byte color = state->expColor++%3 < 2 ? 2 : 3;

      textureRenderEllipseQB(state->buildingsTexture, NULL, state->bombPos.x, state->bombPos.y, state->expC, color, -1.0f);
      state->expC += inc;

      if (state->expC >= radius) {
         state->explosionOut = false;
      }
   }
   else {
      textureRenderEllipseQB(state->buildingsTexture, NULL, state->bombPos.x, state->bombPos.y, state->expC, 0, -1.0f);
      state->expC -= inc;
   }

}


static PlotData _plotShot(SplashState *state) {
   int power = state->power[state->player];
   int angle = state->angle[state->player];
   Int2 gPos = { state->gPos[state->player].x + 14,  state->gPos[state->player].y-3 };

   if (state->player) { angle = 180 - angle; }

   float rad = angle * (PI / 180.0f);

   float initXVel = cosf(rad)*power;
   float initYVel = sinf(rad)*power;

   int oldX = gPos.x;
   int oldY = gPos.y;

   int adjust = 4;

   int xedge = 9 * (1 - state->player);

   bool impact = false;
   bool shotInSun = false;
   bool onScreen = true;
   int playerHit = 0;
   
   int startXPos = gPos.x;
   int startYPos = gPos.y;

   int direction = 0;

   if (state->player) {
      //startXPos += 25;
      direction = 4;
   }
   else {
      direction = -4;
   }

   PlotData out = { 0 };

   out.initXVel = initXVel;
   out.initYVel = initYVel;
   out.onScreen = true;
   out.startXPos = startXPos;
   out.startYPos = startYPos;

   return out;

   //if (power < 2) {
   //   return;//too slow
   //}

   //float t = 0.0f;
   //while (!impact && onScreen) {
   //   int x = startXPos + (initXVel * t) + (0.5f * (state->wind / 5.0f) * t * t);
   //   int y = startYPos + ((-1 * (initYVel * t)) + (0.5f * state->gravity * t * t)) * (EGA_RES_HEIGHT / 350.0f);

   //   if (x >= EGA_RES_WIDTH - 10 || x <= 3 || y >= EGA_RES_HEIGHT - 3) {
   //      onScreen = false;
   //   }

   //   if (onScreen && y > 0) {
   //      byte color = textureGetColorAt(state->buildingsTexture, NULL, x, y);

   //      if (color == 5 || color == 6 || color == 7) {
   //         _doExplosion(state, x, y);
   //         return;
   //      }

   //      textureRenderPoint(state->buildingsTexture, NULL, x, y, 9);
   //   }

   //   t += 0.001f;
   //}
}

static void _updateShot(SplashState *state, PlotData *p) {
   int x = p->startXPos + (p->initXVel * p->t) + (0.5f * (state->wind / 5.0f) * p->t * p->t);
   int y = p->startYPos + ((-1 * (p->initYVel * p->t)) + (0.5f * state->gravity * p->t * p->t)) * (EGA_RES_HEIGHT / 350.0f);

   if (x >= EGA_RES_WIDTH - 10 || x <= 3 || y >= EGA_RES_HEIGHT - 3) {
      p->onScreen = false;
   }

   state->bombPos = (Int2) { x, y };

   if (p->onScreen && y > 0) {
      int lookX, lookY;

      for (lookY = -3; lookY < 3; ++lookY) {
         for (lookX = -3; lookX < 3; ++lookX) {
            byte color = textureGetColorAt(state->buildingsTexture, NULL, x+lookX, y+lookY);

            if (color == 3 && x > EGA_RES_WIDTH/2 - 30 && x < EGA_RES_WIDTH / 2 + 30) {
               state->sunShock = true;
            }

            if (color == 5 || color == 6 || color == 7) {
               p->impact = true;
               break;
            }

            if (color == 1) {
               state->kill = true;
            }
         }
      }
   }
}

static void throwBomb(SplashState *state){
   state->time = appGetTime(appGet());
   state->turnState = Throw;

   state->plotData = _plotShot(state);

   refreshGorilla(state, state->player ? LeftArm : RightArm);
}

static void _drawWindArrow(SplashState *state, Texture *frame) {
   int windLine = state->wind * 3 * (EGA_RES_WIDTH / 320.0f);

   textureRenderLine(frame, NULL, EGA_RES_WIDTH / 2, EGA_RES_HEIGHT - 6, EGA_RES_WIDTH / 2 + windLine, EGA_RES_HEIGHT - 6, 2);

   int arrowDir = state->wind > 0 ? -1 : 2;

   textureRenderLine(frame, NULL, EGA_RES_WIDTH / 2 + windLine, EGA_RES_HEIGHT - 6, EGA_RES_WIDTH / 2 + windLine + arrowDir, EGA_RES_HEIGHT - 6-2, 2);
   textureRenderLine(frame, NULL, EGA_RES_WIDTH / 2 + windLine, EGA_RES_HEIGHT - 6, EGA_RES_WIDTH / 2 + windLine + arrowDir, EGA_RES_HEIGHT - 6+2, 2);


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
#define MAX_POWER 100
#define MS_TO_MAX 2000

static void gbUpdateGame(SplashState *state) {
   if (state->turnState == GetPower) {
      Milliseconds delta = t_u2m(appGetTime(appGet()) - state->time);

      if (delta > MS_TO_MAX) {
         state->power[state->player] = MAX_POWER;
         throwBomb(state);
      }
      else {
         double d = delta / (double)MS_TO_MAX;
         state->power[state->player] = MAX_POWER * d;
      }
   }
   else if (state->turnState == Throw) {
      Milliseconds delta = t_u2m(appGetTime(appGet()) - state->time);
      int updateCount = MIN(delta / 100, 10);
      state->time += t_m2u(updateCount * 100);

      while (updateCount-- >= 0 && !state->plotData.impact) {
         state->plotData.t += 0.05f;
         _updateShot(state, &state->plotData);
      }

      if (state->kill) {
         state->explosionOut = true;
         state->expC = 0.0f;
         state->expRadius = EGA_RES_HEIGHT / 5.0f;
         state->turnState = Explode;
         state->time = appGetTime(appGet());
         refreshGorilla(state, NoArms);
         state->sunShock = false;

      }
      else if (state->plotData.impact) {
         state->explosionOut = true;
         state->expC = 0.0f;
         state->expRadius = EGA_RES_HEIGHT / 30.0f;
         state->turnState = Explode;
         state->time = appGetTime(appGet());
         refreshGorilla(state, NoArms);
         state->sunShock = false;
      }
      else if ((!rectiContains((Recti) { 0, 0, EGA_RES_WIDTH - 1, EGA_RES_HEIGHT - 1 }, state->bombPos) && state->bombPos.y > 0)) {
         state->turnState = GetAngle;
         refreshGorilla(state, NoArms);
         state->player = !state->player;
         state->time = appGetTime(appGet());
         state->sunShock = false;
      }
   }

   else if (state->turnState == Explode) {
      Milliseconds delta = t_u2m(appGetTime(appGet()) - state->time);
      int updateCount = MIN(delta / 10, 1000);
      state->time += t_m2u(updateCount * 10);

      while (updateCount-- >= 0 && state->expC >= 0.0f) {
         updateExplosion(state);
      }

      if (state->expC < 0.0f) {
         if (state->kill) {
            state->player = state->bombPos.x > EGA_RES_WIDTH / 2 ? 0 : 1;

            state->turnState = Win;
            state->time = appGetTime(appGet());
         }
         else {
            state->turnState = GetAngle;
            state->player = !state->player;
            state->time = appGetTime(appGet());
         }

         
      }
   }
   else if (state->turnState == Win) {
      Milliseconds delta = t_u2m(appGetTime(appGet()) - state->time);
      refreshGorilla(state, LeftArm + (delta / 500) % 2);
      if (delta > 4000) {
         _startGame(state);
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

   _drawWindArrow(state, frame);
}


static void _drawAngledLine(Texture *frame, FrameRegion *vp, int xc, int yc, int len, int angle, byte color, byte xFlipped) {
   float rad = angle * (PI / 180.0f);
   Int2 pos2 = {
      xFlipped ? xc - cosf(rad)*len : cosf(rad)*len + xc,
      yc - sinf(rad)*len
   };

   textureRenderLine(frame, vp, xc, yc, pos2.x, pos2.y, color);
}

static void _drawAngledArrow(Texture *frame, FrameRegion *vp, int xc, int yc, int len, int angle, byte color, byte xFlipped) {
   float rad = angle * (PI / 180.0f);
   Int2 pos2 = {  
      xFlipped ? xc - cosf(rad)*len : cosf(rad)*len + xc,
      yc - sinf(rad)*len
   };

   textureRenderLine(frame, vp, xc, yc, pos2.x, pos2.y, color);
   _drawAngledLine(frame, vp, pos2.x, pos2.y, 10, angle - 180 + 45, color, xFlipped);
   _drawAngledLine(frame, vp, pos2.x, pos2.y, 10, angle - 180 - 45, color, xFlipped);
}

static void _drawAngleArrow(SplashState *state, Texture *frame) {
   Int2 gPos = state->gPos[state->player];
   int angle = state->angle[state->player];

   _drawAngledArrow(frame, NULL, gPos.x + 14, gPos.y-3, 50, angle, 3, state->player);

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

   //dosun
   textureRenderRect(state->buildingsTexture, NULL, (EGA_RES_WIDTH - 41) / 2, 25 - 31 / 2, (EGA_RES_WIDTH - 41) / 2 + 41, 25 - 31 / 2 + 31, 0);
   if (state->sunShock) {
      Texture *sun = managedImageGetTexture(state->sun2);
      textureRenderTexture(state->buildingsTexture, NULL, (EGA_RES_WIDTH - textureGetWidth(sun)) / 2, 25 - textureGetHeight(sun) / 2, sun);
   }
   else {
      Texture *sun = managedImageGetTexture(state->sun);
      textureRenderTexture(state->buildingsTexture, NULL, (EGA_RES_WIDTH - textureGetWidth(sun)) / 2, 25 - textureGetHeight(sun) / 2, sun);
   }

   //do buildings/gorillas
   if (state->buildingsTexture) {
      textureRenderTexture(frame, NULL, 0, 0, state->buildingsTexture);
   }

   //do UI
   _drawGameUI(state, frame);

   if (state->turnState < Throw) {
      _drawAngleArrow(state, frame);
      _drawPowerBar(state, frame);
   }
   else if (state->turnState == Throw) {
      Texture *bomb = managedImageGetTexture(state->bomb);
      textureRenderTexture(frame, NULL, state->bombPos.x - textureGetWidth(bomb) / 2, state->bombPos.y - textureGetHeight(bomb) / 2, bomb);
   }


   if (state->sunShock) {
      Texture *sun = managedImageGetTexture(state->sun2);
      textureRenderTexture(frame, NULL, (EGA_RES_WIDTH - textureGetWidth(sun)) / 2, 25 - textureGetHeight(sun) / 2, sun);
   }
   else {
      Texture *sun = managedImageGetTexture(state->sun);
      textureRenderTexture(frame, NULL, (EGA_RES_WIDTH - textureGetWidth(sun)) / 2, 25 - textureGetHeight(sun) / 2, sun);
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
         case SegaKey_Space:
            if (state->turnState == GetPower) {
               throwBomb(state);
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


static void gbUpdateSelect(SplashState *state) {
   
}
static void gbRenderSelect(SplashState *state, GameStateRender *m) {
   Texture *frame = m->frame;
   int gWidth = 40;
   int spacing = 16;
   int i;

   int gStartX = (EGA_RES_WIDTH - (gWidth * 4) - (spacing * 3)) / 2;
   int bestCoastY = EGA_TEXT_CHAR_HEIGHT * 4;
   int beastCoastY = EGA_TEXT_CHAR_HEIGHT * 12;
   int gravY = EGA_TEXT_CHAR_HEIGHT * 20;


   textureClear(frame, NULL, 0);

   textAreaRender(state->txt, state->view, frame);

   switch (state->selectSection) {
   case 0:textureRenderLineRect(frame, NULL, gStartX + state->selectOption * (gWidth + spacing), bestCoastY - 5, gStartX + state->selectOption * (gWidth + spacing) + gWidth, bestCoastY + 35, 7);
      break;
   case 1:textureRenderLineRect(frame, NULL, gStartX + state->selectOption * (gWidth + spacing), beastCoastY - 5, gStartX + state->selectOption * (gWidth + spacing) + gWidth, beastCoastY + 35, 7);
      break;
   case 2:textureRenderLineRect(frame, NULL, gStartX + state->selectOption * (gWidth + spacing) - 3, gravY - 5, gStartX + state->selectOption * (gWidth + spacing) + gWidth + 5, gravY + 20, 7);
      break;
   }

   
   
   for (i = 0; i < 4; ++i) {
      bool westAnim = (i == state->selectOption && state->selectSection == 0) || (state->selectSection != 0 && i == state->duders[0] - Drew);
      bool eastAnim = (i == state->selectOption && state->selectSection == 1) || (state->selectSection != 1 && i == state->duders[1] - Dan);
      Milliseconds t = t_u2m(appGetTime(appGet()));

      _drawGorilla(state, frame, gStartX + 6 + i * (gWidth + spacing), bestCoastY, westAnim ? LeftArm + (t / 300) % 2 :  NoArms, Drew + i);
      _drawGorilla(state, frame, gStartX + 6 + i * (gWidth + spacing), beastCoastY, eastAnim ? LeftArm + (t / 300) % 2 : NoArms, Dan + i);
   }

   Font *nameFont = fontFactoryGetFont(state->view->fontFactory, 0, 2);
   textureRenderText(frame, " DREW   BRAD   JEFF  JASON", gStartX / EGA_TEXT_CHAR_WIDTH, 7, nameFont);
   textureRenderText(frame, " DAN   VINNY  ALEX   JEFF B", gStartX / EGA_TEXT_CHAR_WIDTH, 15, nameFont);


   textureRenderText(frame, "EARTH", gStartX / EGA_TEXT_CHAR_WIDTH, 20, nameFont);
   textureRenderText(frame, "MOON", gStartX / EGA_TEXT_CHAR_WIDTH + 8, 20, nameFont);
   textureRenderText(frame, "MARS", gStartX / EGA_TEXT_CHAR_WIDTH + 15, 20, nameFont);
   textureRenderText(frame, "SUN", gStartX / EGA_TEXT_CHAR_WIDTH + 22, 20, nameFont);

   
}
static void _saveChoice(SplashState *state) {
   if (state->selectSection == 0) {//west coast
      switch (state->selectOption) {
      case 0:stringSet(state->pNames[0], "Drew"); state->duders[0] = Drew;break;
      case 1:stringSet(state->pNames[0], "Brad"); state->duders[0] = Brad;break;
      case 2:stringSet(state->pNames[0], "Jeff"); state->duders[0] = Jeff;break;
      case 3:stringSet(state->pNames[0], "Jason-kun"); state->duders[0] = Jason;break;
      }
   }
   else if (state->selectSection == 1) {//east coast
      switch (state->selectOption) {
      case 0:stringSet(state->pNames[1], "Dirty Dan"); state->duders[1] = Dan;break;
      case 1:stringSet(state->pNames[1], "Vinny"); state->duders[1] = Vinny;break;
      case 2:stringSet(state->pNames[1], "Alex"); state->duders[1] = Alex;break;
      case 3:stringSet(state->pNames[1], "Bakalar"); state->duders[1] = Jeff2;break;
      }
   }
   else {
      switch (state->selectOption) {
      case 0:state->gravity = 9.81f;break;
      case 1:state->gravity = 1.622f;break;
      case 2:state->gravity = 3.711f;break;
      case 3:state->gravity = 274.87f;break;
      }
   }
}

static void gbHandleKeyboardSelect(SplashState *state) {
   WorldView *view = state->view;
   Keyboard *k = appGetKeyboard(appGet());
   KeyboardEvent e = { 0 };

   while (keyboardPopEvent(k, &e)) {
      if (e.action == SegaKey_Released) {
         if (e.key == SegaKey_Left) {
            state->selectOption = MAX(state->selectOption - 1, 0);
         }

         if (e.key == SegaKey_Right) {
            state->selectOption = MIN(state->selectOption + 1, 3);            
         }

         if (e.key == SegaKey_Enter || e.key == SegaKey_Space) {
            _saveChoice(state);
            ++state->selectSection;
            state->selectOption = 0;
            if (state->selectSection > 2) {
               _playGame(state);
               return;
            }
         }
      }
   }
}




static void _splashStateCreate(SplashState *state) {
   int i = 0;
   for (i = 0; i < 2; ++i) {
      state->pNames[i] = stringCreate("");
   }

   state->txt = textAreaCreate(0, 0, EGA_TEXT_RES_WIDTH, EGA_TEXT_RES_HEIGHT);   
   textAreaSetJustify(state->txt, TextAreaJustify_Center);

   state->sun = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gorilla-sun"));
   state->sun2 = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gorilla-sun2"));
   state->gorilla = imageLibraryGetImage(state->view->imageLibrary, stringIntern("gorilla2"));
   state->bomb = imageLibraryGetImage(state->view->imageLibrary, stringIntern("bomb"));


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
   managedImageDestroy(self->sun2);
   managedImageDestroy(self->gorilla);
   managedImageDestroy(self->bomb);
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
      "\n\n\n\n"
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
   case Characters:
      gbUpdateSelect(state);
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
   case Characters:
      gbHandleKeyboardSelect(state);
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
   case Characters:
      gbRenderSelect(state, m);
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