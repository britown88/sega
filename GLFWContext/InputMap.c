#include "GLFW/glfw3.h"

#include "SEGA\Input.h"
#include "segautils\Defs.h"
#include <string.h>

int getSegaAction(int GLFWAction){
   static SegaInputActions actionMap[3];
   static bool mapInit = false;

   if (!mapInit){
      actionMap[GLFW_PRESS] = SegaKey_Pressed;
      actionMap[GLFW_RELEASE] = SegaKey_Released;
      actionMap[GLFW_REPEAT] = SegaKey_Repeat;

      mapInit = true;
   }

   return actionMap[GLFWAction];
}

int getSegaKey(int GLFWKey){
   static SegaKeys keyMap[GLFW_KEY_LAST];
   static bool mapInit = false;

   if (!mapInit){
      int i = 0; 
      for (i = 0; i < GLFW_KEY_LAST; ++i){ keyMap[i] = -1; }

      keyMap[GLFW_KEY_SPACE] = SegaKey_Space;
      keyMap[GLFW_KEY_APOSTROPHE] = SegaKey_Apostrophe;
      keyMap[GLFW_KEY_COMMA] = SegaKey_Comma;
      keyMap[GLFW_KEY_MINUS] = SegaKey_Minus;
      keyMap[GLFW_KEY_PERIOD] = SegaKey_Period;
      keyMap[GLFW_KEY_SLASH] = SegaKey_Slash;
      keyMap[GLFW_KEY_0] = SegaKey_0;
      keyMap[GLFW_KEY_1] = SegaKey_1;
      keyMap[GLFW_KEY_2] = SegaKey_2;
      keyMap[GLFW_KEY_3] = SegaKey_3;
      keyMap[GLFW_KEY_4] = SegaKey_4;
      keyMap[GLFW_KEY_5] = SegaKey_5;
      keyMap[GLFW_KEY_6] = SegaKey_6;
      keyMap[GLFW_KEY_7] = SegaKey_7;
      keyMap[GLFW_KEY_8] = SegaKey_8;
      keyMap[GLFW_KEY_9] = SegaKey_9;
      keyMap[GLFW_KEY_SEMICOLON] = SegaKey_Semicolon;
      keyMap[GLFW_KEY_EQUAL] = SegaKey_Equal;
      keyMap[GLFW_KEY_A] = SegaKey_A;
      keyMap[GLFW_KEY_B] = SegaKey_B;
      keyMap[GLFW_KEY_C] = SegaKey_C;
      keyMap[GLFW_KEY_D] = SegaKey_D;
      keyMap[GLFW_KEY_E] = SegaKey_E;
      keyMap[GLFW_KEY_F] = SegaKey_F;
      keyMap[GLFW_KEY_G] = SegaKey_G;
      keyMap[GLFW_KEY_H] = SegaKey_H;
      keyMap[GLFW_KEY_I] = SegaKey_I;
      keyMap[GLFW_KEY_J] = SegaKey_J;
      keyMap[GLFW_KEY_K] = SegaKey_K;
      keyMap[GLFW_KEY_L] = SegaKey_L;
      keyMap[GLFW_KEY_M] = SegaKey_M;
      keyMap[GLFW_KEY_N] = SegaKey_N;
      keyMap[GLFW_KEY_O] = SegaKey_O;
      keyMap[GLFW_KEY_P] = SegaKey_P;
      keyMap[GLFW_KEY_Q] = SegaKey_Q;
      keyMap[GLFW_KEY_R] = SegaKey_R;
      keyMap[GLFW_KEY_S] = SegaKey_S;
      keyMap[GLFW_KEY_T] = SegaKey_T;
      keyMap[GLFW_KEY_U] = SegaKey_U;
      keyMap[GLFW_KEY_V] = SegaKey_V;
      keyMap[GLFW_KEY_W] = SegaKey_W;
      keyMap[GLFW_KEY_X] = SegaKey_X;
      keyMap[GLFW_KEY_Y] = SegaKey_Y;
      keyMap[GLFW_KEY_Z] = SegaKey_Z;
      keyMap[GLFW_KEY_LEFT_BRACKET] = SegaKey_LeftBracket;
      keyMap[GLFW_KEY_BACKSLASH] = SegaKey_Backslash;
      keyMap[GLFW_KEY_RIGHT_BRACKET] = SegaKey_RightBracket;
      keyMap[GLFW_KEY_GRAVE_ACCENT] = SegaKey_GraveAccent;
      keyMap[GLFW_KEY_ESCAPE] = SegaKey_Escape;
      keyMap[GLFW_KEY_ENTER] = SegaKey_Enter;
      keyMap[GLFW_KEY_TAB] = SegaKey_Tab;
      keyMap[GLFW_KEY_BACKSPACE] = SegaKey_Backspace;
      keyMap[GLFW_KEY_INSERT] = SegaKey_Insert;
      keyMap[GLFW_KEY_DELETE] = SegaKey_Delete;
      keyMap[GLFW_KEY_RIGHT] = SegaKey_Right;
      keyMap[GLFW_KEY_LEFT] = SegaKey_Left;
      keyMap[GLFW_KEY_DOWN] = SegaKey_Down;
      keyMap[GLFW_KEY_UP] = SegaKey_Up;
      keyMap[GLFW_KEY_PAGE_UP] = SegaKey_PageUp;
      keyMap[GLFW_KEY_PAGE_DOWN] = SegaKey_PageDown;
      keyMap[GLFW_KEY_HOME] = SegaKey_Home;
      keyMap[GLFW_KEY_END] = SegaKey_End;
      keyMap[GLFW_KEY_CAPS_LOCK] = SegaKey_CapsLock;
      keyMap[GLFW_KEY_SCROLL_LOCK] = SegaKey_ScrollLock;
      keyMap[GLFW_KEY_NUM_LOCK] = SegaKey_NumLock;
      keyMap[GLFW_KEY_PRINT_SCREEN] = SegaKey_PrintScreen;
      keyMap[GLFW_KEY_PAUSE] = SegaKey_Pause;
      keyMap[GLFW_KEY_F1] = SegaKey_F1;
      keyMap[GLFW_KEY_F2] = SegaKey_F2;
      keyMap[GLFW_KEY_F3] = SegaKey_F3;
      keyMap[GLFW_KEY_F4] = SegaKey_F4;
      keyMap[GLFW_KEY_F5] = SegaKey_F5;
      keyMap[GLFW_KEY_F6] = SegaKey_F6;
      keyMap[GLFW_KEY_F7] = SegaKey_F7;
      keyMap[GLFW_KEY_F8] = SegaKey_F8;
      keyMap[GLFW_KEY_F9] = SegaKey_F9;
      keyMap[GLFW_KEY_F10] = SegaKey_F10;
      keyMap[GLFW_KEY_F11] = SegaKey_F11;
      keyMap[GLFW_KEY_F12] = SegaKey_F12;
      keyMap[GLFW_KEY_KP_0] = SegaKey_Keypad0;
      keyMap[GLFW_KEY_KP_1] = SegaKey_Keypad1;
      keyMap[GLFW_KEY_KP_2] = SegaKey_Keypad2;
      keyMap[GLFW_KEY_KP_3] = SegaKey_Keypad3;
      keyMap[GLFW_KEY_KP_4] = SegaKey_Keypad4;
      keyMap[GLFW_KEY_KP_5] = SegaKey_Keypad5;
      keyMap[GLFW_KEY_KP_6] = SegaKey_Keypad6;
      keyMap[GLFW_KEY_KP_7] = SegaKey_Keypad7;
      keyMap[GLFW_KEY_KP_8] = SegaKey_Keypad8;
      keyMap[GLFW_KEY_KP_9] = SegaKey_Keypad9;
      keyMap[GLFW_KEY_KP_DECIMAL] = SegaKey_KeypadDecimal;
      keyMap[GLFW_KEY_KP_DIVIDE] = SegaKey_KeypadDivide;
      keyMap[GLFW_KEY_KP_MULTIPLY] = SegaKey_KeypadMultiply;
      keyMap[GLFW_KEY_KP_SUBTRACT] = SegaKey_KeypadSubtract;
      keyMap[GLFW_KEY_KP_ADD] = SegaKey_KeypadAdd;
      keyMap[GLFW_KEY_KP_ENTER] = SegaKey_KeypadEnter;
      keyMap[GLFW_KEY_KP_EQUAL] = SegaKey_KeypadEqual;
      keyMap[GLFW_KEY_LEFT_SHIFT] = SegaKey_LeftShift;
      keyMap[GLFW_KEY_LEFT_CONTROL] = SegaKey_LeftControl;
      keyMap[GLFW_KEY_LEFT_ALT] = SegaKey_LeftAlt;
      keyMap[GLFW_KEY_RIGHT_SHIFT] = SegaKey_RightShift;
      keyMap[GLFW_KEY_RIGHT_CONTROL] = SegaKey_RightControl;
      keyMap[GLFW_KEY_RIGHT_ALT] = SegaKey_RightAlt;

      mapInit = true;
   }

   return keyMap[GLFWKey];
}