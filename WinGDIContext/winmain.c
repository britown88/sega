////////////////////////////////////////////
////                                      //
//// OpenGL in a PROPER Windows APP       //
//// ( NO GLUT !! )                       //
////                                      //
//// You found this at bobobobo's weblog, //
//// https://bobobobo.wordpress.com        //
////                                      //
//// Creation date:  Feb 9/08             //
//// Last modified:  Feb 10/08            //
////                                      //
////////////////////////////////////////////
//
//#include <windows.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <math.h>
//#include <gl/gl.h>
//#include <gl/glu.h>
//
//#pragma comment(lib, "opengl32.lib")
//#pragma comment(lib, "glu32.lib")
//
//struct Globals
//{
//   HINSTANCE hInstance;    // window app instance
//
//   HWND hwnd;      // handle for the window
//
//   HDC   hdc;      // handle to device context
//
//   HGLRC hglrc;    // handle to OpenGL rendering context
//
//   int width, height;      // the desired width and
//};
//
//Globals g;
//
//LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow);
//void draw(); 
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
//{
//   g.hInstance = hInstance;
//
//   WNDCLASS wc;
//   wc.cbClsExtra = 0;
//   wc.cbWndExtra = 0;
//   wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
//   wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//   wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
//   wc.hInstance = hInstance;
//   wc.lpfnWndProc = WndProc;
//   wc.lpszClassName = TEXT("Philip");
//   wc.lpszMenuName = 0;
//   wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
//
//   // Register that class with the Windows O/S..
//   RegisterClass(&wc);
//
//
//   RECT rect;
//   SetRect(&rect, 50,  // left
//      50,  // top
//      850, // right
//      650); // bottom
//
//   // Save width and height off.
//   g.width = rect.right - rect.left;
//   g.height = rect.bottom - rect.top;
//
//   // Adjust it.
//   AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
//
//   g.hwnd = CreateWindow(TEXT("Philip"),
//      TEXT("GL WINDOW!"),
//      WS_OVERLAPPEDWINDOW,
//      rect.left, rect.top,  // adjusted x, y positions
//      rect.right - rect.left, rect.bottom - rect.top,  // adjusted width and height
//      NULL, NULL,
//      hInstance, NULL);
//
//   if (g.hwnd == NULL)
//   {
//      FatalAppExit(NULL, TEXT("CreateWindow() failed!"));
//   }
//
//   ShowWindow(g.hwnd, iCmdShow);
//
//   g.hdc = GetDC(g.hwnd);
//
//   PIXELFORMATDESCRIPTOR pfd = { 0 }; 
//
//   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
//   pfd.nVersion = 1;
//
//   pfd.dwFlags = PFD_SUPPORT_OPENGL |
//      PFD_DOUBLEBUFFER |
//      PFD_DRAW_TO_WINDOW;
//
//   pfd.iPixelType = PFD_TYPE_RGBA;
//   pfd.cColorBits = 24;
//
//   pfd.cDepthBits = 32;
//
//   int chosenPixelFormat = ChoosePixelFormat(g.hdc, &pfd);
//
//   if (chosenPixelFormat == 0)
//   {
//      FatalAppExit(NULL, TEXT("ChoosePixelFormat() failed!"));
//   }
//
//   char b[100];
//   sprintf(b, "You got ID# %d as your pixelformat!\n", chosenPixelFormat);
//   MessageBoxA(NULL, b, "Your pixelformat", MB_OK);
//
//   int result = SetPixelFormat(g.hdc, chosenPixelFormat, &pfd);
//
//   if (result == NULL)
//   {
//      FatalAppExit(NULL, TEXT("SetPixelFormat() failed!"));
//   }
//
//   g.hglrc = wglCreateContext(g.hdc);
//   wglMakeCurrent(g.hdc, g.hglrc);
//
//   MSG msg;
//
//   while (1)
//   {
//      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//      {
//         if (msg.message == WM_QUIT)
//         {
//            break;
//         }
//
//         TranslateMessage(&msg);
//         DispatchMessage(&msg);
//      }
//      else
//      {
//         draw();
//      }
//   }
//#pragma endregion
//
//   //////////////
//   // clean up
//#pragma region clean up
//   // UNmake your rendering context (make it 'uncurrent')
//   wglMakeCurrent(NULL, NULL);
//
//   // Delete the rendering context, we no longer need it.
//   wglDeleteContext(g.hglrc);
//
//   // release your window's DC
//   ReleaseDC(g.hwnd, g.hdc);
//#pragma endregion
//
//   // and a cheesy fade exit
//   AnimateWindow(g.hwnd, 200, AW_HIDE | AW_BLEND);
//
//   return msg.wParam;
//}
//
//////////////////////////
//// DRAWING FUNCTION
//void draw()
//{
//   // 1. set up the viewport
//   glViewport(0, 0, g.width, g.height); // set viewport
//   // to be the whole width and height
//   // of the CLIENT AREA (drawable region) of the window,
//   // (the CLIENT AREA excludes the titlebar and the 
//   // maximize/minimize buttons).
//
//   // 2. projection matrix
//   glMatrixMode(GL_PROJECTION);
//   glLoadIdentity();
//   gluPerspective(45.0, (float)g.width / (float)g.height, 1, 1000);
//
//   // 3. viewing transformation
//   glMatrixMode(GL_MODELVIEW);
//   glLoadIdentity();
//
//   gluLookAt(0, 0, 10,
//      0, 0, 0,
//      0, 1, 0);
//
//   // 4. modelling transformation and drawing
//   glClearColor(0.5, 0, 0, 0);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//   static float i = 0.01f;
//   // Notice that 'i' is a STATIC variable.
//   // That's very important. (imagine me saying
//   // that like Conchords in "Business Time")
//   // http://youtube.com/watch?v=WGOohBytKTU
//
//   // A 'static' variable is created ONCE
//   // when the function in which it sits first runs.
//
//   // The static variable will "LIVE ON"
//   // between seperate calls to the function
//   // in which it lives UNTIL THE PROGRAM ENDS.
//
//   i += 0.001f;     // increase i by 0.001 from its
//   // it had on the LAST FUNCTION CALL to the draw() function
//
//   float c = cos(i);
//   float s = sin(i);
//
//   glBegin(GL_TRIANGLES);
//   glColor3f(c, 0, 0);      // red
//   glVertex3f(1 + c, 0 + s, 0);
//
//   glColor3f(c, s, 0);      // yellow
//   glVertex3f(0 + c, 1 + s, 0);
//
//   glColor3f(s, 0.1f, s);   // magenta
//   glVertex3f(-1 + c, 0 + s, 0);
//   glEnd();
//
//   //7.  SWAP BUFFERS.
//   SwapBuffers(g.hdc);
//   // Its important to realize that the backbuffer
//   // is intelligently managed by the HDC ON ITS OWN,
//   // so all's you gots to do is call SwapBuffers
//   // on the HDC of your window.
//}
//
//
//
//
//
//////////////////////////
//// WNDPROC
//// Notice that WndProc is very very neglected.
//// We hardly do anything with it!  That's because
//// we do all of our processing in the draw()
//// function.
//LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
//{
//   switch (message)
//   {
//   case WM_CREATE:
//      Beep(50, 10);
//      return 0;
//      break;
//
//   case WM_PAINT:
//   {
//      HDC hdc;
//      PAINTSTRUCT ps;
//      hdc = BeginPaint(hwnd, &ps);
//      // don't draw here.  would be waaay too slow.
//      // draw in the draw() function instead.
//      EndPaint(hwnd, &ps);
//   }
//   return 0;
//   break;
//
//   case WM_KEYDOWN:
//      switch (wparam)
//      {
//      case VK_ESCAPE:
//         PostQuitMessage(0);
//         break;
//      default:
//         break;
//      }
//      return 0;
//
//   case WM_DESTROY:
//      PostQuitMessage(0);
//      return 0;
//      break;
//   }
//
//   return DefWindowProc(hwnd, message, wparam, lparam);
//}
//
//
//
//////////////////
//// END NOTES:
////
//// Some good references:
//// WGL function ref on MSDN:
//// http://msdn2.microsoft.com/en-us/library/ms673957%28VS.85%29.aspx
//
//// MSDN example from 1994 (but still good!)
//// "OpenGL I: Quick Start"
//// http://msdn2.microsoft.com/en-us/library/ms970745.aspx
//
//
////////////////////
//// QUICK Q&A (make sure you know what's going on):
////
//// QUESTION:  What's the means by which we can draw
////            to our window itself?
////
//// ANSWER:  The HDC (HANDLE TO DEVICE CONTEXT).
//
//// QUESTION:  What's the means by which OpenGL can
////            draw to the window?
////
//// ANSWER:  USING that SAME HDC WE woulda used
////          to draw to it!! (more to come on this now).
//
///////////////////////
//// It IS possible to access the bits
//// of the output of OpenGL in 2 ways:
////      1)  Use glReadPixels() to obtain
////          the arrayful of pixels on the
////          screen.  You can then save this
////          to a .TGA or .BMP file easily.
//
////      2)  Render to a BITMAP, then
////          blit that bitmap to your HDC.
////          There's a 1995 msdn article on this:
////          "OpenGL VI: Rendering on DIBs with PFD_DRAW_TO_BITMAP"
////          http://msdn2.microsoft.com/en-us/library/ms970768.aspx
//
//
//
///*
//____   __   __      __   __  ___
/// _  \ /  / /  /    /  /  \ \/  /
/// _/ / /  / /  /    /  /    \   /
/// _/ \ /  / /  /__  /  /__   /  /
///_____//__/ /______//______/ /__/
//
//*/