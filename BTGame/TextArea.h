#pragma once

#include "segashared/Strings.h"
#include "GameClock.h"

typedef struct Frame_t Frame;
typedef struct WorldView_t WorldView;

typedef struct TextArea_t TextArea;
typedef struct TextAreaManager_t TextAreaManager;

TextAreaManager *textAreaManagerCreate(WorldView *view);
void textAreaManagerDestroy(TextAreaManager *self);
TextArea *textAreaManagerGet(TextAreaManager *self, StringView id);

//registers a text area with a global id inside the worldview, assumes allocation-ownership
//this is automatically mirrored to lua
void textAreaManagerRegister(TextAreaManager *self, StringView id, TextArea *area);

TextArea *textAreaCreate(short x, short y, short width, short height);
void textAreaDestroy(TextArea *self);
void textAreaSetSpeed(TextArea *self, Milliseconds timePerCharacter);
void textAreaPushText(TextArea *self, const char *msg);
void textAreaSetText(TextArea *self, const char *msg);//like push but clears current queue and current drawn, also forces an instant update (ideal for 0-speed)
bool textAreaIsDone(TextArea *self);
void textAreaSetVisibility(TextArea *self, bool visible);
void textAreaHide(TextArea *self);
void textAreaShow(TextArea *self);

void textAreaUpdate(TextArea *self);
void textAreaRender(TextArea *self, WorldView *view, Frame *frame);