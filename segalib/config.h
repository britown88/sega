#pragma once

/*
   Define your video mode here:
   
   Directive      |  Frame Res      |  Text Mode
   ************************************************
   EGA_MODE_0Dh   |  320x200x16c    |  8x8  (40x25)
   EGA_MODE_10h   |  640x350x16c    |  8x14 (80x25)

*/
#define EGA_MODE_10h


/*
this re-exposes all the old framerender calls
*/
//#define USE_DEPRECATED_RENDER_API

