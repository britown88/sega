#pragma once

#include "../libpng/png.h"
#include "CheckedMemory.h"

#include <string>
#include <memory>
#include <vector>
#include <iostream>

static unsigned char ckR = 164;
static unsigned char ckG = 64;
static unsigned char ckB = 164;

namespace {

struct Color
{
   unsigned char r,g,b,a;

};

struct PNGImage
{
   PNGImage(){}
   PNGImage(int x, int y)
   {
      width = x;
      height = y;
      image_data.reset(new Color[width * height]);
      memset(image_data.get(), 0, sizeof(Color)*width*height);
   }
   PNGImage(PNGImage&& rhs)
      : image_data(std::move(rhs.image_data)), width(rhs.width), height(rhs.height)
   {

   }
   PNGImage(PNGImage const& rhs)
      : width(rhs.width), height(rhs.height)
   {
      image_data.reset(new Color[width * height]);
      memcpy(image_data.get(), rhs.image_data.get(), width * height * sizeof(Color));
   }
   std::unique_ptr<Color[]> image_data;
   int width, height;

   
};


PNGImage loadPng(std::string const& textureFile)
{
   FILE* infile = fopen(textureFile.c_str(), "rb"); 
   if (!infile) {
      throw std::exception("failed to load texture.");
   }

   unsigned char sig[8];
   fread(sig, 1, 8, infile);
   if (!png_check_sig(sig, 8)) {
      fclose(infile);
      throw std::exception("failed to load texture.");
   }

   png_structp png_ptr; 
   png_infop info_ptr; 
   png_infop end_ptr; 

   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (!png_ptr) {
      fclose(infile);
      throw std::exception("failed to load texture.");
   }

   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      fclose(infile);
      throw std::exception("failed to load texture.");
   }

   end_ptr = png_create_info_struct(png_ptr);
   if (!end_ptr) {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
      fclose(infile);
      throw std::exception("failed to load texture.");
   }

   if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, &end_ptr);
      fclose(infile);
      throw std::exception("failed to load texture.");
   }

   png_ptr->io_ptr = (png_voidp)infile;
   png_set_sig_bytes(png_ptr, 8);    


   int  bit_depth;
   int  color_type;

   unsigned long width;
   unsigned long height;
   unsigned int rowbytes;

   png_read_info(png_ptr, info_ptr);
   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, 
      &color_type, NULL, NULL, NULL);

   png_read_update_info(png_ptr, info_ptr);
   rowbytes = png_get_rowbytes(png_ptr, info_ptr);

   if (bit_depth > 8) {
      png_set_strip_16(png_ptr);
   }
   if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
         png_set_gray_to_rgb(png_ptr);
   }
   if (color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png_ptr);
   }

   Color* image_data;
   png_bytepp row_pointers = NULL;

   size_t totalSize = rowbytes*height;
   bool noAlpha = (rowbytes / width) == 3;
   if (noAlpha)
   {
      totalSize = (totalSize * 4) / 3;
   }

   if ((image_data =  new Color[totalSize/4])==NULL) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      throw std::exception("failed to load texture.");
   }

   if ((row_pointers = (png_bytepp)checkedMalloc(height*sizeof(png_bytep))) == NULL) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      checkedFree(image_data);
      image_data = NULL;
      throw std::exception("failed to load texture.");
   }

   for (unsigned int i = 0;  i < height;  ++i)
   {
      row_pointers[i] = (unsigned char*)(image_data) + i*(rowbytes);    
   }

   png_read_image(png_ptr, row_pointers);

   checkedFree(row_pointers);
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
   fclose(infile);


   if (noAlpha)
   {
      unsigned char* actual = ((unsigned char*)image_data) + totalSize-1;
      unsigned char* original = ((unsigned char*)image_data) + ((totalSize/4) * 3)-1;
      for (unsigned int i = 0; i < width * height; ++i)
      {
         *actual-- = 255;          //a 
         *actual-- = *original--;  //b 
         *actual-- = *original--;  //g
         *actual-- = *original--;  //r
      }
   }

   PNGImage out;
   out.width = width;
   out.height = height;
   out.image_data.reset(image_data);
   return out;
}
void savePng(PNGImage& image, std::string file_name)
{
   /* create file */
   FILE *fp = fopen(file_name.c_str(), "wb");
   if (!fp)
      throw std::exception("Couldn't open file for reading.");


   /* initialize stuff */
   auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (!png_ptr)
      throw std::exception("png_create_write_struct failed");

   auto info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
      throw std::exception("png_create_info_struct failed");

   if (setjmp(png_jmpbuf(png_ptr)))
      throw std::exception("Error during init_io");

   png_init_io(png_ptr, fp);


   /* write header */
   if (setjmp(png_jmpbuf(png_ptr)))
      throw std::exception("Error during writing header");

   png_set_IHDR(png_ptr, info_ptr, image.width, image.height,
      8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
      PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);


   /* write bytes */
   if (setjmp(png_jmpbuf(png_ptr)))
      throw std::exception("Error during writing bytes");

   std::unique_ptr<png_bytep[]> row_pointers(new png_bytep[image.height]);


   int rowbytes = image.width * 4; 
   for (int i = 0;  i < image.height;  ++i)
      row_pointers[i] = (unsigned char*)(image.image_data.get() + i*(rowbytes/4));    

   png_write_image(png_ptr, row_pointers.get());

   /* end write */
   if (setjmp(png_jmpbuf(png_ptr)))
      throw std::exception("Error during end of write");

   png_write_end(png_ptr, NULL);

   fclose(fp);
}

template <typename T> 
void runOperation(std::string path, T&& fn)
{
   try
   {
      std::cout << "Processing file " << path << "...";
      auto img = loadPng(path);
      fn(img);
      savePng(img, path);
      std::cout << " Done!" << std::endl;
   }
   catch (...)
   {
      std::cout << "Encountered an issue when converting file " << path << std::endl;
   }
}

template <typename T> 
void runPNGImageOperationOnDirectory(std::string directory, T&& fn)
{
   for (auto file : getFilesInDirectory(directory))
   {
      runOperation(file, fn);
   }
   for (auto directory: getSubdirectoriesInDirectory(directory))
   {
      runPNGImageOperationOnDirectory(directory, fn);
   }
}

template <typename T> 
void runPNGImageOperation(int argc, char**argv, T&& fn)
{
   if (argc < 2) 
   {
      std::cout << "To use this, drag a file or directory over." << std::endl;
      system("Pause");
      return;
   }
   if (fileIsDirectory(argv[1]))
   {
      runPNGImageOperationOnDirectory(argv[1], fn);
   }
   else
   {
      runOperation(argv[1], fn);
   }

   system("Pause");
}

}