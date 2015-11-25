#include "TextHelpers.h"
#include "segautils/Defs.h"

//// >=D
byte textGenerateColorCode(byte bg, byte fg) { 
   return (bg & 15) | ((fg & 15) << 4); 
}

void textExtractColorCode(byte c, byte *bg, byte *fg) { 
   *bg = c & 15; 
   *fg = (c >> 4) & 15; 
}

static char _colorCodeFromText(byte bg, byte fg) {
   byte abg = 0, afg = 0;
   if (bg >= '0' && bg <= '9') { abg = bg - '0'; }
   else if (bg >= 'A' && bg <= 'F') {  abg = bg - 'A' + 10; }
   else if (bg >= 'a' && bg <= 'f') { abg = bg - 'a' + 10; }

   if (fg >= '0' && fg <= '9') { afg = fg - '0'; }
   else if (fg >= 'A' && fg <= 'F') { afg = fg - 'A' + 10; }
   else if (fg >= 'a' && fg <= 'f') { afg = fg - 'a' + 10; }

   return textGenerateColorCode(abg, afg);
}

void stringRenderToArea(const char *str, size_t lineWidth, vec(StringPtr) *outList) {
   char *msg = (char*)str;
   char c = 0;
   static char buff[256] = { 0 };
   size_t index = 0;
   size_t lastSpace = 0;
   size_t skippedChars = 0;
   size_t skippedSinceLastSpace = 0;
   size_t spaceOnLastSkip = 0;
   byte lastColor = 0;
   bool colorChanged = false;

   while (c = *msg++) {
      switch (c) {
      case '\r':
         break;
      case '\\':
         c = *msg++;
         if (c && c == 'c') {
            byte bg = *msg++;
            byte fg = *msg++;
            buff[index++] = '\\';
            buff[index++] = 'c';
            buff[index] = _colorCodeFromText(bg, fg);
            lastColor = buff[index];
            colorChanged = true;
            skippedChars += 3;

            if (spaceOnLastSkip != lastSpace) {
               spaceOnLastSkip = lastSpace;
               skippedSinceLastSpace = 0;
            }
            skippedSinceLastSpace += 3;
         }
         break;
      case '\n':
         lastSpace = index;
         break;
      case '\t':
         lastSpace = index;
         break;
      case ' ':
         lastSpace = index;
      default:
         buff[index] = c;
         break;
      }
      ++index;
      if (index - skippedChars >= lineWidth) {
         String *str;
         buff[lastSpace] = 0;
         str = stringCreate(buff);
         vecPushBack(StringPtr)(outList, &str);
         index -= lastSpace + 1;
         memcpy(buff, buff + lastSpace + 1, index);

         if (lastSpace == spaceOnLastSkip) {
            skippedChars = skippedSinceLastSpace;
         }
         else {
            if (colorChanged) {               
               skippedChars = 3;
               memmove(buff + 3, buff, index);
               buff[0] = '\\';
               buff[1] = 'c';
               buff[2] = lastColor;
               index += 3;
            }
            else {
               skippedChars = 0;
            }
         }

         //reset crap
         lastSpace = 0;         
         skippedSinceLastSpace = 0;
         spaceOnLastSkip = 0;
      }
   }

   if (index - skippedChars > 0) {
      String *str;
      buff[index] = 0;
      str = stringCreate(buff);
      vecPushBack(StringPtr)(outList, &str);
   }
}