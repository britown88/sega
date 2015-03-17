//#include "WinGDIContext.h"
//
//#include "SEGA\IDeviceContext.h"
//#include "SEGA\App.h"
//
//#include "segashared/CheckedMemory.h"
//
//#include <stdio.h>
//#ifndef API_ENTRY
//#include <windows.h>
//#endif
//
//typedef struct {
//   IDeviceContext context;
//   HWND hWnd;
//   HDC hDC;
//   HGLRC hGLRC;
//   HPALETTE hPalette;
//   int quit;
//   int width;
//   int height;
//}WinGDIContext;
//
//static WinGDIContext *g_window;
//
//static int _init(WinGDIContext *self, int width, int height, StringView winTitle, int flags);
//static void _preRender(WinGDIContext *self);
//static void _postRender(WinGDIContext *self);
//static int _shouldClose(WinGDIContext *self);
//static Int2 _windowSize(WinGDIContext *self);
//static double _time(WinGDIContext *self);
//static void _destroy(WinGDIContext *self);
//
//static IDeviceContextVTable *_getTable(){
//   static IDeviceContextVTable *out = NULL;
//   if (!out){
//      out = calloc(1, sizeof(IDeviceContextVTable));
//      out->init = (int(*)(IDeviceContext*, int, int, StringView, int))&_init;
//      out->preRender = (void(*)(IDeviceContext*))&_preRender;
//      out->postRender = (void(*)(IDeviceContext*))&_postRender;
//      out->shouldClose = (int(*)(IDeviceContext*))&_shouldClose;
//      out->windowSize = (Int2(*)(IDeviceContext*))&_windowSize;
//      out->time = (double(*)(IDeviceContext*))&_time;
//      out->destroy = (void(*)(IDeviceContext*))&_destroy;
//   }
//   return out;
//}
//
//IDeviceContext *createWinGDIContext(){
//   WinGDIContext *out = checkedCalloc(1, sizeof(WinGDIContext));
//   out->context.vTable = _getTable();
//   return (IDeviceContext *)out;
//}
//
//LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
//
//int _init(WinGDIContext *self, int width, int height, StringView winTitle, int flags){
//   HINSTANCE hCurrentInst = GetModuleHandle(NULL);
//   WNDCLASS wndClass;
//   HWND hWnd;
//   char *className = "sega";
//
//   g_window = self;
//
//   /* register window class */
//   wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
//   wndClass.lpfnWndProc = WndProc;
//   wndClass.cbClsExtra = 0;
//   wndClass.cbWndExtra = 0;
//   wndClass.hInstance = hCurrentInst;
//   wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
//   wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
//   wndClass.hbrBackground = GetStockObject(BLACK_BRUSH);
//   wndClass.lpszMenuName = NULL;
//   wndClass.lpszClassName = className;
//   RegisterClass(&wndClass);
//
//   /* create window */
//   hWnd = CreateWindow(
//      className, winTitle,
//      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
//      0, 0, width, height,
//      NULL, NULL, hCurrentInst, NULL);
//
//   if (!hWnd){
//      return 1;
//   }
//
//   g_window->hWnd = hWnd;
//
//   /* display window */
//   ShowWindow(hWnd, TRUE);
//   UpdateWindow(hWnd);
//
//   return 0;
//}
//void _preRender(WinGDIContext *self){
//
//}
//void _postRender(WinGDIContext *self){
//   MSG msg;
//   
//
//   if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//   {
//      TranslateMessage(&msg);
//      DispatchMessage(&msg);
//   }
//
//   SwapBuffers(g_window->hDC);
//
//
//}
//int _shouldClose(WinGDIContext *self){
//   return self->quit;
//}
//Int2 _windowSize(WinGDIContext *self){
//   return (Int2){self->width, self->height};
//}
//double _time(WinGDIContext *self){
//   return (double)GetTickCount() / 1000.0;
//}
//void _destroy(WinGDIContext *self){
//
//   checkedFree(self);
//}
//
//void setupPixelFormat(HDC hDC) {
//   PIXELFORMATDESCRIPTOR pfd = {
//      sizeof(PIXELFORMATDESCRIPTOR),  /* size */
//      1,                              /* version */
//      PFD_SUPPORT_OPENGL |
//      PFD_DRAW_TO_WINDOW |
//      PFD_DOUBLEBUFFER,               /* support double-buffering */
//      PFD_TYPE_RGBA,                  /* color type */
//      16,                             /* prefered color depth */
//      0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
//      0,                              /* no alpha buffer */
//      0,                              /* alpha bits (ignored) */
//      0,                              /* no accumulation buffer */
//      0, 0, 0, 0,                     /* accum bits (ignored) */
//      16,                             /* depth buffer */
//      0,                              /* no stencil buffer */
//      0,                              /* no auxiliary buffers */
//      PFD_MAIN_PLANE,                 /* main layer */
//      0,                              /* reserved */
//      0, 0, 0,                        /* no layer, visible, damage masks */
//   };
//   int pixelFormat;
//
//   pixelFormat = ChoosePixelFormat(hDC, &pfd);
//   if (pixelFormat == 0) {
//      MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
//         MB_ICONERROR | MB_OK);
//      exit(1);
//   }
//
//   if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
//      MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
//         MB_ICONERROR | MB_OK);
//      exit(1);
//   }
//}
//
//void setupPalette(HDC hDC) {
//   int pixelFormat = GetPixelFormat(hDC);
//   PIXELFORMATDESCRIPTOR pfd;
//   LOGPALETTE* pPal;
//   int paletteSize;
//
//   DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
//
//   if (pfd.dwFlags & PFD_NEED_PALETTE) {
//      paletteSize = 1 << pfd.cColorBits;
//   }
//   else {
//      return;
//   }
//
//   pPal = (LOGPALETTE*)
//      malloc(sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY));
//   pPal->palVersion = 0x300;
//   pPal->palNumEntries = paletteSize;
//
//   /* build a simple RGB color palette */
//   {
//      int redMask = (1 << pfd.cRedBits) - 1;
//      int greenMask = (1 << pfd.cGreenBits) - 1;
//      int blueMask = (1 << pfd.cBlueBits) - 1;
//      int i;
//
//      for (i = 0; i<paletteSize; ++i) {
//         pPal->palPalEntry[i].peRed =
//            (((i >> pfd.cRedShift) & redMask) * 255) / redMask;
//         pPal->palPalEntry[i].peGreen =
//            (((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask;
//         pPal->palPalEntry[i].peBlue =
//            (((i >> pfd.cBlueShift) & blueMask) * 255) / blueMask;
//         pPal->palPalEntry[i].peFlags = 0;
//      }
//   }
//
//   g_window->hPalette = CreatePalette(pPal);
//   free(pPal);
//
//   if (g_window->hPalette) {
//      SelectPalette(hDC, g_window->hPalette, FALSE);
//      RealizePalette(hDC);
//   }
//}
//
//
//LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
//   switch (message) {
//   case WM_CREATE:
//      /* initialize OpenGL rendering */
//      g_window->hDC = GetDC(hWnd);
//      setupPixelFormat(g_window->hDC);
//      setupPalette(g_window->hDC);
//      g_window->hGLRC = wglCreateContext(g_window->hDC);
//      wglMakeCurrent(g_window->hDC, g_window->hGLRC);
//      return 0;
//   case WM_DESTROY:
//      /* finish OpenGL rendering */
//      if (g_window->hGLRC) {
//         wglMakeCurrent(NULL, NULL);
//         wglDeleteContext(g_window->hGLRC);
//      }
//      if (g_window->hPalette) {
//         DeleteObject(g_window->hPalette);
//      }
//      ReleaseDC(hWnd, g_window->hDC);
//      PostQuitMessage(0);
//      g_window->quit = TRUE;
//      return 0;
//   case WM_SIZE:
//      /* track window size changes */
//      if (g_window->hGLRC) {
//         g_window->width = (int)LOWORD(lParam);
//         g_window->height = (int)HIWORD(lParam);
//         return 0;
//      }
//   case WM_PALETTECHANGED:
//      /* realize palette if this is *not* the current window */
//      if (g_window->hGLRC && g_window->hPalette && (HWND)wParam != hWnd) {
//         UnrealizeObject(g_window->hPalette);
//         SelectPalette(g_window->hDC, g_window->hPalette, FALSE);
//         RealizePalette(g_window->hDC);
//         SwapBuffers(g_window->hDC);
//         break;
//      }
//      break;
//   case WM_QUERYNEWPALETTE:
//      /* realize palette if this is the current window */
//      if (g_window->hGLRC && g_window->hPalette) {
//         UnrealizeObject(g_window->hPalette);
//         SelectPalette(g_window->hDC, g_window->hPalette, FALSE);
//         RealizePalette(g_window->hDC);
//         SwapBuffers(g_window->hDC);
//         return TRUE;
//      }
//      break;
//   case WM_PAINT:
//   {
//      PAINTSTRUCT ps;
//      BeginPaint(hWnd, &ps);
//      EndPaint(hWnd, &ps);
//      return 0;
//   }
//   break;
//   case WM_CHAR:
//      /* handle keyboard input */
//      switch ((int)wParam) {
//      case VK_ESCAPE:
//         DestroyWindow(hWnd);
//         g_window->quit = TRUE;
//         return 0;
//      default:
//         break;
//      }
//      break;
//   default:
//      break;
//   }
//   return DefWindowProc(hWnd, message, wParam, lParam);
//}
