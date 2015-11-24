#pragma once

#include "segautils\DLLBullshit.h"
#include "segautils\Vector.h"

#define SegaKey_Undefined -1

typedef enum{
   SegaMouseBtn_Left = 0,
   SegaMouseBtn_Right,
   SegaMouseBtn_Middle,
   SegaMouseBtn_4,
   SegaMouseBtn_5,
   SegaMouseBtn_6,
   SegaMouseBtn_7,
   SegaMouseBtn_8,
   SegaMouseBtn_COUNT
}SegaMouseButtons;

typedef enum{
   SegaMouse_Pressed = 0,
   SegaMouse_Released,
   SegaMouse_Moved,
   SegaMouse_Scrolled
}SegaMouseActions;

typedef enum {
   SegaKey_Pressed= 0,
   SegaKey_Released,
   SegaKey_Repeat,
   SegaKey_Char
}SegaKeyActions;

typedef enum{
   SegaKey_Space = 0,
   SegaKey_Apostrophe,
   SegaKey_Comma,
   SegaKey_Minus,
   SegaKey_Period,
   SegaKey_Slash,
   SegaKey_0,
   SegaKey_1,
   SegaKey_2,
   SegaKey_3,
   SegaKey_4,
   SegaKey_5,
   SegaKey_6,
   SegaKey_7,
   SegaKey_8,
   SegaKey_9,
   SegaKey_Semicolon,
   SegaKey_Equal,
   SegaKey_A,
   SegaKey_B,
   SegaKey_C,
   SegaKey_D,
   SegaKey_E,
   SegaKey_F,
   SegaKey_G,
   SegaKey_H,
   SegaKey_I,
   SegaKey_J,
   SegaKey_K,
   SegaKey_L,
   SegaKey_M,
   SegaKey_N,
   SegaKey_O,
   SegaKey_P,
   SegaKey_Q,
   SegaKey_R,
   SegaKey_S,
   SegaKey_T,
   SegaKey_U,
   SegaKey_V,
   SegaKey_W,
   SegaKey_X,
   SegaKey_Y,
   SegaKey_Z,
   SegaKey_LeftBracket,
   SegaKey_Backslash,
   SegaKey_RightBracket,
   SegaKey_GraveAccent,
   SegaKey_Escape,
   SegaKey_Enter,
   SegaKey_Tab,
   SegaKey_Backspace,
   SegaKey_Insert,
   SegaKey_Delete,
   SegaKey_Right,
   SegaKey_Left,
   SegaKey_Down,
   SegaKey_Up,
   SegaKey_PageUp,
   SegaKey_PageDown,
   SegaKey_Home,
   SegaKey_End,
   SegaKey_CapsLock,
   SegaKey_ScrollLock,
   SegaKey_NumLock,
   SegaKey_PrintScreen,
   SegaKey_Pause,
   SegaKey_F1,
   SegaKey_F2,
   SegaKey_F3,
   SegaKey_F4,
   SegaKey_F5,
   SegaKey_F6,
   SegaKey_F7,
   SegaKey_F8,
   SegaKey_F9,
   SegaKey_F10,
   SegaKey_F11,
   SegaKey_F12,
   SegaKey_Keypad0,
   SegaKey_Keypad1,
   SegaKey_Keypad2,
   SegaKey_Keypad3,
   SegaKey_Keypad4,
   SegaKey_Keypad5,
   SegaKey_Keypad6,
   SegaKey_Keypad7,
   SegaKey_Keypad8,
   SegaKey_Keypad9,
   SegaKey_KeypadDecimal,
   SegaKey_KeypadDivide,
   SegaKey_KeypadMultiply,
   SegaKey_KeypadSubtract,
   SegaKey_KeypadAdd,
   SegaKey_KeypadEnter,
   SegaKey_KeypadEqual,
   SegaKey_LeftShift,
   SegaKey_LeftControl,
   SegaKey_LeftAlt,
   SegaKey_RightShift,
   SegaKey_RightControl,
   SegaKey_RightAlt,
   SegaKey_COUNT
}SegaKeys;

/*----Mouse -------*/
typedef struct {
   SegaMouseActions action;
   SegaMouseButtons button;
   Int2 pos;
}MouseEvent;

#define ClosureTPart \
    CLOSURE_RET(Int2) \
    CLOSURE_NAME(MousePos) \
    CLOSURE_ARGS()
#include "segautils\Closure_Decl.h"

typedef struct Mouse_t Mouse;

DLL_PUBLIC Mouse *mouseCreate(MousePos posFunc);
DLL_PUBLIC void mouseDestroy(Mouse *self);

DLL_PUBLIC void mousePushEvent(Mouse *self, MouseEvent *event);
DLL_PUBLIC Int2 mouseGetPosition(Mouse *self);

//return if succeeded (false if empty)
DLL_PUBLIC int mousePopEvent(Mouse *self, MouseEvent *eventOut);
DLL_PUBLIC int mouseIsDown(Mouse *self, SegaMouseButtons button);
DLL_PUBLIC void mouseFlushQueue(Mouse *self);


/*----Keyboard -------*/
typedef struct {
   SegaKeyActions action;
   SegaKeys key;
   unsigned int unichar;
}KeyboardEvent;

typedef struct Keyboard_t Keyboard;

DLL_PUBLIC Keyboard *keyboardCreate();
DLL_PUBLIC void keyboardDestroy(Keyboard *self);

DLL_PUBLIC void keyboardPushEvent(Keyboard *self, KeyboardEvent *event);

//return if succeeded (false if empty)
DLL_PUBLIC int keyboardPopEvent(Keyboard *self, KeyboardEvent *eventOut);
DLL_PUBLIC int keyboardIsDown(Keyboard *self, SegaKeys key);
DLL_PUBLIC void keyboardFlushQueue(Keyboard *self);