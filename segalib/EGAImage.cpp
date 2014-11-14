#include "ImageTools.hpp"

#include <direct.h>
#include <stdio.h>

#include <algorithm>

#include <list>

#include "EGA.h"
#include "ColorMath.hpp"
#include "EGAImage.hpp"
#include "bt-utils\BitTwiddling.h"

#define MAX_PALETTE_SIZE 16

class EGAImage::Impl {
   byte *m_pixels, *m_alpha;
   byte m_palette[16];
   short m_width, m_height;

   int m_colorCounts[64];//how many of each ega color is in the image
   byte *m_pixelMap;//has an ega value for every pixel
public:
   Impl(Image *image, byte *palette) {
      m_width = imageGetWidth(image);
      m_height = imageGetHeight(image);

      m_pixels = new byte[m_width * m_height];
      m_alpha = new byte[m_width * m_height];
      m_pixelMap = new byte[m_width * m_height];

      memset(m_colorCounts, 0, sizeof(int)*64);
      memset(m_pixels, 0, m_width * m_height);
      memset(m_alpha, 0, m_width * m_height);
      memset(m_pixelMap, 0, m_width * m_height);

      memcpy(m_palette, palette, 16);

      SuperScanLine lines[EGA_IMAGE_PLANES];

      for(int y = 0; y < m_height; ++y) {
         for(int i = 0; i < EGA_IMAGE_PLANES; ++i) {
            auto sl = imageGetScanLine(image, y, i);
            imageScanLineRender(sl, lines[i].pixels);
         }

         for(int x = 0; x < m_width; ++x) {
            byte alpha = getBitFromArray(lines[0].pixels, x);
            if(alpha) {
               m_alpha[y*m_width+x] = 1;

               byte color = 0;
               for(int i = 0; i < EGA_PLANES; ++i) {
                  color += (getBitFromArray(lines[i+1].pixels, x) << i);
               }

               m_pixels[y*m_width+x] = color;               
               m_pixelMap[y*m_width+x] = m_palette[color];
               ++m_colorCounts[m_palette[color]];
            }            
         }
      }
   }


   Impl(const char *file)
   {
      auto img = loadPng(file);

      m_width = img.width;
      m_height = img.height;      

      m_pixels = new byte[m_width * m_height];
      m_alpha = new byte[m_width * m_height];
      m_pixelMap = new byte[m_width * m_height];

      memset(m_palette, 0, 16);
      memset(m_colorCounts, 0, sizeof(int)*64);
      memset(m_pixels, 0, m_width * m_height);
      memset(m_alpha, 0, m_width * m_height);
      memset(m_pixelMap, 0, m_width * m_height);

      std::vector<int> cArray(img.width * img.height);

      //push every pixel into a vector
      for(int i = 0; i < img.width * img.height; ++i) {
         m_alpha[i] = img.image_data[i].a == 255;
         cArray[i] = *(int*)&img.image_data[i];
      }

      //sort and unique
      std::sort(begin(cArray), end(cArray));
      cArray.erase(std::unique(cArray.begin(), cArray.end()), cArray.end());

      //map the unique colors to their ega equivalents
      std::vector<rgbega> colorMap(cArray.size());

      for(unsigned int i = 0; i < cArray.size(); ++i)
         colorMap[i] = rgbega(cArray[i], closestEGA(cArray[i]));

      //go throuygh the image and log how often each EGA color appears
      for(int i = 0; i < img.width * img.height; ++i) {
         int c = *(int*)&img.image_data[i];

         if(img.image_data[i].a != 255){
            m_pixelMap[i] = 0;
            continue;
         }

         byte ega = std::lower_bound(begin(colorMap), end(colorMap), c)->ega;

         m_pixelMap[i] = ega;
         m_colorCounts[ega]++;
      }
   }

   ~Impl(){ 
      delete [] m_pixels;
      delete [] m_alpha;
      delete [] m_pixelMap;
   }

   short width(){return m_width;}
   short height(){return m_height;}

   void removeColorEntries(PaletteColor *removedColor, ImageColor *colors){
      for (auto& entry : removedColor->entries)
      {
         for (auto color = colors; color != colors+64; ++color)
         {
            if (color->closestColor == &entry)  
            {
               color->closestColor = entry.next;  
            }
         }
         if (entry.prev) entry.prev->next = entry.next;
         if (entry.next) entry.next->prev = entry.prev;
      }
   }

   void renderWithPalette(byte *p) {

      byte forced[64];
      memset(forced, EGA_COLOR_UNUSED, 64);

      size_t totalCount = 0;
      for(int i = 0; i < 16; ++i) {
         if(p[i] != EGA_COLOR_UNUSED) {
            if(p[i] != EGA_COLOR_UNDEFINED) {
               forced[p[i]] = totalCount;
            }

            ++totalCount;
         }
      }

      std::list<PaletteColor> palette;
      ImageColor colors[64];
      for (int i = 0; i < 64; ++i)
      {
         if(forced[i] != EGA_COLOR_UNUSED) {
            palette.push_back(PaletteColor(i, forced[i]));
         }else {
            palette.push_back(PaletteColor(i));
         }
         
         for (int j = 0; j < 64; ++j)
         {
            insertSortedPaletteEntry(colors[j],  palette.back(), j, i, palette.back().entries[j]);
         }
      }

       while (palette.size() > totalCount)
      {
         //worst color, worst error...
         float lowestDistance = FLT_MAX;
         std::list<PaletteColor>::iterator rarestColor;
 
         for (auto color = palette.begin(); color != palette.end(); ++color) //do this with iterators to erase.
         {
            float distance= 0.0f;
            for (auto& entry : color->entries)
            {
               if (isClosest(colors[color->EGAColor], entry))
               {
                  distance += m_colorCounts[color->EGAColor] * (entry.next->distance - entry.distance);
               }
            }
            if (distance < lowestDistance && color->removable)
            {
               lowestDistance = distance;
               rarestColor = color;
            }
         }
 
         //remove rarest color, and all of its palette entries in the colors array....
         removeColorEntries(&*rarestColor, colors);
         palette.erase(rarestColor);
      }

       //eliminate unused colors
       for (auto color = palette.begin(); color != palette.end();) {
          if(color->removable) {
            color->distance= 0.0f;

            for (auto& entry : color->entries) {
               if (isClosest(colors[color->EGAColor], entry)) {
                  color->distance += m_colorCounts[color->EGAColor] * (entry.next->distance - entry.distance);
               }
            }

            if(color->distance == 0.0f){
               //color wasnt used
               removeColorEntries(&*color, colors);
               color = palette.erase(color);
               continue;
            }  
          }     
          ++color;
       }

       palette.sort([&](const PaletteColor&r1, const PaletteColor&r2){return r1.distance > r2.distance;});

 
      //this also gives you the look-up table on output...
   
      byte paletteOut[MAX_PALETTE_SIZE];
      memset(paletteOut, EGA_COLOR_UNUSED, MAX_PALETTE_SIZE);
      int LUTcolor = 0;

      //two passes, first to inster colors who have locked positions in the palette
      for (auto& color : palette) {
         if(!color.removable) {
            paletteOut[color.pPos] = color.EGAColor;
            color.EGAColor = color.pPos;
         }
      }

      //next is to fill in the blanks with the rest
      for (auto& color : palette)
      {
         if(color.removable) {
            while(paletteOut[LUTcolor] != EGA_COLOR_UNUSED){LUTcolor += 1;};
            paletteOut[LUTcolor] = color.EGAColor;
            color.EGAColor = LUTcolor++;
         }
      }
 
      byte colorLUT[64]; //look-up table from 64 colors down the 16 remaining colors.
      LUTcolor = 0;
      for (auto& color : colors)
      {
         colorLUT[LUTcolor++] = color.closestColor->color->EGAColor;
      }
  
      memcpy(m_palette, paletteOut, 16);

      //now set the image data from the color map
      for(int i = 0; i < m_width * m_height; ++i) {
         m_pixels[i] = colorLUT[m_pixelMap[i]];
      }
   }
   
   byte *palette(){ return m_palette;}
   Image *toImage(){
      int x, y, j;
      Image *img = imageCreate(m_width, m_height);

      SuperScanLine lines[EGA_IMAGE_PLANES] = {0};

      for(y = 0; y < m_height; ++y){
         for(x = 0; x < m_width;++x) {
            byte pixel=  m_pixels[y*m_width+x];
            byte alpha = m_alpha[y*m_width+x];

            scanLineSetBit((ScanLine*)&lines[0], x, alpha);

            if(alpha) {
               for(j = 0; j < EGA_PLANES; ++j) {
                  scanLineSetBit((ScanLine*)&lines[j+1], x, !!(pixel&(1<<j)));
               }
            } 
         }

         for(j = 0; j < EGA_IMAGE_PLANES; ++j) {
            imageSetScanLine(img, y, j, createSmallestScanLine(m_width, lines[j].pixels));
         }
      }

      return img;
   }

   void exportPNG(const char*path){
      PNGImage img(m_width, m_height);

      for(int i = 0; i < m_width * m_height; ++i) {
         if(m_alpha[i]){
            img.image_data[i] = EGAColorLookup(m_palette[m_pixels[i]]);
         }
      }

      savePng(img, path);
   }
};

EGAImage::EGAImage(const char *file):pImpl(new Impl(file)){}
EGAImage::EGAImage(Image *image, byte *palette):pImpl(new Impl(image, palette)){}
EGAImage::~EGAImage(){}

short EGAImage::width(){return pImpl->width();}
short EGAImage::height(){return pImpl->height();}

void EGAImage::renderWithPalette(byte *p){pImpl->renderWithPalette(p);}

byte *EGAImage::palette(){ return pImpl->palette();}
Image *EGAImage::toImage(){ return pImpl->toImage();}

void EGAImage::exportPNG(const char*path){pImpl->exportPNG(path);}


