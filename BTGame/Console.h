#pragma once

#include "segautils/Defs.h"
#include "SEGA/Input.h"
#include "WorldView.h"

typedef struct Texture_t Texture;
typedef struct Console_t Console;
typedef struct Actor_t Actor;

Console *consoleCreate(WorldView *view);
void consoleDestroy(Console *self);
void consoleUpdate(Console *self);

void consoleCreateLines(Console *self);
void consoleSetEnabled(Console *self, bool enabled);
bool consoleGetEnabled(Console *self);
void consoleInputChar(Console *self, char c);
void consoleInputKey(Console *self, SegaKeys key, SegaKeyMods mods);
void consolePushLine(Console *self, const char *line);
void consoleClear(Console *self);
void consoleScrollLog(Console *self, int direction);
void consoleMacroGridPos(Console *self, short x, short y);
void consoleMacroActor(Console *self, Actor *a);
void consolePrintLuaError(Console *self, const char *tag);

void consoleRenderNotification(Console *self, Texture *tex);
void consoleRenderLines(Console *self, Texture *tex);

#define consolePrintLine(CONSOLE, STR, ...) {\
   char buffer[256] = {0}; \
   sprintf(buffer, STR, __VA_ARGS__ ); \
   consolePushLine(CONSOLE, buffer); \
}

#define consolePrintError(CONSOLE, STR, ...) {\
   char buffer[256] = {0}; \
   sprintf(buffer, STR, __VA_ARGS__ ); \
   consolePrintLine(CONSOLE, "[c=0,13][=]%s[/=][/c]", buffer); \
}